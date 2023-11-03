/**
 * Asignatura Sistemas Concurrentes y Distribuidos
 * Practica 2
 * @file fumadoresSU.cpp
 * @author Miguel Torres Alonso
 * @brief Implementación del problema de los fumadores con un monitor SU
 * @version 0.1
 * @date 2023-11-03
 */
// Compilar: ~# g++ -std=c++11 -pthread -I. -o fumadoresSU_exe fumadoresSU.cpp scd.cpp


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_fumadores = 3;

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

int producirIngrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a 'duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}


class Estanco : public HoareMonitor
{
 private:
 static const int              // constantes ('static' ya que no dependen de la instancia)
   num_ingredientes = 3;       // núm. de entradas del buffer

 CondVar                       // colas condicion:
   ingr_disp[num_ingredientes],                      // cola donde espera el fumador
   mostr_vacio;                // cola donde espera el estanquero

 public:                       // constructor y métodos públicos
   Estanco() ;                 // constructor
   // Funciones que llama el estanquero
   void ponerIngrediente(int ingrediente);
   void esperarRecogidaIngrediente();

   // Funcion que llaman los fumadores
   void obtenerIngrediente(int ingrediente);
} ;

Estanco::Estanco()
{
    for(int i = 0; i < num_ingredientes; i++){
        ingr_disp[i] = newCondVar();
    }
    mostr_vacio = newCondVar();
}


void Estanco::ponerIngrediente(int ingrediente)
{
    cout << "Puesto el ingrediente numero:	" << ingrediente << "	en el mostrador." << endl;
    ingr_disp[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente()
{
    cout << "Soy estanquero y me espero a que retiren ingrediente del mostrador." << endl;
    mostr_vacio.wait();
}

void Estanco::obtenerIngrediente(int ingrediente)
{
    cout << "Soy fumador y espero el ingrediente numero:	" << ingrediente << endl;
    ingr_disp[ingrediente].wait();
    mostr_vacio.signal();
    cout << "Soy fumador y acabo de retirar el ingrediente numero:     " << ingrediente << endl; 
}


//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> estanco)
{
	int ingrediente;
	while(true)
	{
	    ingrediente = producirIngrediente();
        estanco->ponerIngrediente(ingrediente);
		estanco->esperarRecogidaIngrediente();
	}
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
// varios fumadores pueden fumar simultaneamente
void  funcion_hebra_fumador(  MRef<Estanco> estanco, int num_fumador )
{
   while( true )
   {
		estanco->obtenerIngrediente(num_fumador);
		fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{		
    // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
    MRef<Estanco> estanco = Create<Estanco>() ;

	thread array_heb_fumadores[num_fumadores];
	thread hebra_estanquero;
	
	hebra_estanquero = thread(funcion_hebra_estanquero, estanco);
	for(unsigned i = 0; i < num_fumadores; i++)
	{
		array_heb_fumadores[i] = thread(funcion_hebra_fumador, estanco, i);
	}
	
	hebra_estanquero.join();
	for(unsigned i = 0; i < num_fumadores; i++)
	{
		array_heb_fumadores[i].join();
	}
}
