/**
 * Asignatura: Sistemas Concurrentes y Distribuidos
 * Practica 2
 * @file lectoresEscritoresSU.cpp
 * @author Miguel Torres Alonso
 * @brief Implementación del problema de los lectores-escritores con un monitor SU
 * @version 0.1
 * @date 2023-11-03
 */
// Compilar: ~# g++ -std=c++11 -pthread -I. -o lecEsc_exe lectoresEscritoresSU.cpp scd.cpp


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_lectores = 4, 
        num_escritores = 6;


void retrasoAleatorio()
{
    // calcular milisegundos aleatorios de duración
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // espera bloqueada un tiempo igual a 'duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );
}


class Lec_Esc : public HoareMonitor
{
 private:

  // variables permanentes
  bool escrib;                    // true si hay algun escritor escribiendo
  int n_lec;                    // numero de lectores leyendo
 
 CondVar                            // colas condicion:
   lectura,                         // no hay escrit. escribiendo, lectura posible
   escritura;                      // no hay lect. ni escrit., escritura posible

 public:                            // constructor y métodos públicos
   Lec_Esc() ;                      // constructor

   void ini_lectura();
   void fin_lectura();

   void ini_escritura();
   void fin_escritura();

} ;

// Constructor
Lec_Esc::Lec_Esc()
{
    lectura = newCondVar();
    escritura = newCondVar();
    escrib = false;
    n_lec=0;
}

// Funciones invocadas por lectores
void Lec_Esc::ini_lectura()
{
    if(escrib){lectura.wait();}
    n_lec++;
    lectura.signal();
    cout << "Lector empieza a leer. " << endl;
}

void Lec_Esc::fin_lectura()
{
    cout << "Lector termina de leer. " << endl;
    n_lec--;
    if(n_lec==0){escritura.signal();}
}

// Funciones invocadas por escritores
void Lec_Esc::ini_escritura()
{
    if(escrib || n_lec > 0){escritura.wait();}
    escrib = true;
    cout << "Escritor empieza a escribir. " << endl;
}

void Lec_Esc::fin_escritura()
{
    cout << "Escritor termina de escribir. " << endl;
    escrib = false;
    if(!lectura.empty())
    {
        lectura.signal();
    }
    else
    {
        escritura.signal();
    }   
}

//----------------------------------------------------------------------
// función que ejecutan las hebras lectoras
void funcion_hebra_lector( MRef<Lec_Esc> LecEsc)
{
	while(true)
	{
        LecEsc->ini_lectura();
        retrasoAleatorio();
        LecEsc->fin_lectura();
        retrasoAleatorio();
	}
}

//----------------------------------------------------------------------
// función que ejecutan las hebras escritoras
void  funcion_hebra_escritor( MRef<Lec_Esc> LecEsc )
{
   while( true )
   {
        LecEsc->ini_escritura();
        retrasoAleatorio();
        LecEsc->fin_escritura();
        retrasoAleatorio();
   }
}

//----------------------------------------------------------------------

int main()
{		
    // crear monitor  ('LecEsc' es una referencia al mismo, de tipo MRef<...>)
    MRef<Lec_Esc> lecEsc = Create<Lec_Esc>() ;

	thread array_heb_lectores[num_lectores];
	thread array_heb_escritores[num_escritores];
	
	for(unsigned i = 0; i < num_lectores; i++)
	{
		array_heb_lectores[i] = thread(funcion_hebra_lector, lecEsc);
	}
    for(unsigned i = 0; i < num_escritores; i++)
	{
        array_heb_escritores[i] = thread(funcion_hebra_escritor, lecEsc);
	}
	
	for(unsigned i = 0; i < num_lectores; i++)
	{
		array_heb_lectores[i].join();
	}
    for(unsigned i = 0; i < num_escritores; i++)
	{
		array_heb_escritores[i].join();
	}
}
