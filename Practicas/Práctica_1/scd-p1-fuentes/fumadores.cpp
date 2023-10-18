#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores 

const int num_fumadores = 3;
const int num_ingredientes = 3;

Semaphore ingr_disp[num_ingredientes] = {0,0,0};
Semaphore mostr_vacio(1);

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
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

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
	int ingrediente;
	while(true)
	{
	    ingrediente = producir_ingrediente();
		sem_wait(mostr_vacio);
		cout << "Puesto el ingrediente numero:	" << ingrediente << "	en el mostrador." << endl;
		sem_signal(ingr_disp[ingrediente]);
	}
}

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

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
// varios fumadores pueden fumar simultaneamente
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
   		cout << "Fumador:	" << num_fumador << "	espera ingrediente:	" << num_fumador << endl;
		sem_wait(ingr_disp[num_fumador]);
		cout << "Soy fumador " << num_fumador << " y retiro el ingrediente numero:	" << num_fumador << endl;
		sem_signal(mostr_vacio);
		fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{		
	thread array_heb_fumadores[num_fumadores];
	thread hebra_estanquero;
	
	hebra_estanquero = thread(funcion_hebra_estanquero);
	for(unsigned i = 0; i < num_fumadores; i++)
	{
		array_heb_fumadores[i] = thread(funcion_hebra_fumador, i);
	}
	
	hebra_estanquero.join();
	for(unsigned i = 0; i < num_fumadores; i++)
	{
		array_heb_fumadores[i].join();
	}
}
