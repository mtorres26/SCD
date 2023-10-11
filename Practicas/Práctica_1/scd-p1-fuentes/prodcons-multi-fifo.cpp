#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 20 ,   // número de items
	tam_buffer   = 10,   // tamaño del buffer
   num_heb_productoras = 4,
   num_heb_consumidoras = 4;

unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

int buffer[tam_buffer];
int primera_ocupada = 0, primera_libre = 0;

int vec_productoras[num_heb_productoras];

Semaphore ocupadas(0);
Semaphore libres(tam_buffer);
Semaphore producir(1);
Semaphore consumir(1);
//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(unsigned i)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   //const unsigned dato_producido = i * num_heb_productoras + vec_productoras[i];
   // Cada hebra produce valores entre el rango que le corresponde
   const unsigned dato_producido = i*(num_items/num_heb_productoras) + vec_productoras[i]; 
   vec_productoras[i]++;
   cont_prod[dato_producido]++;
   cout << "Hebra: " << i << " produce el dato: " << dato_producido << endl;
   //cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido;
}
//----------------------------------------------------------------------

void consumir_dato(unsigned dato, unsigned i)
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "Hebra: " << i << " consume el dato: " << dato << endl;
   //cout << "consumido: " << dato << endl << flush;
}
//----------------------------------------------------------------------

void  funcion_hebra_productora(unsigned id_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_heb_productoras ; i++ )
   {
      int dato = producir_dato(id_hebra);
      sem_wait(libres);
      sem_wait(producir);
      buffer[primera_libre] = dato;
      cout << "Introduzco dato: " << dato  << " en posicion: " << primera_libre << endl << flush;
      primera_libre = (primera_libre + 1) % tam_buffer;
      sem_signal(producir);
      sem_signal(ocupadas);
   }
   cout << "Fin de hebra productora numero " << id_hebra << endl;
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(unsigned id_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_heb_consumidoras ; i++ )
   {
      int dato;
      sem_wait(ocupadas);
      sem_wait(consumir);
      dato = buffer[primera_ocupada];
      cout << "Extraigo dato: " << dato << " en posicion: " << primera_ocupada << endl << flush;
      primera_ocupada = (primera_ocupada + 1) % tam_buffer;
      sem_signal(consumir);
      sem_signal(libres);
      consumir_dato(dato, id_hebra) ;
   }
   cout << "Fin de hebra consumidora numero " << id_hebra << endl;
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los multiples productores-consumidores (solución FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   // Inicializo vector en el que las productoras iran sumando valores para producir entre i*p hasta i*p+p-1
   for(int i = 0; i < num_heb_productoras; i++){
      vec_productoras[i] = 0;
   }

   thread array_heb_productoras[num_heb_productoras];
   thread array_heb_consumidoras[num_heb_consumidoras];

   for(int i = 0; i < num_heb_productoras; i++){
      array_heb_productoras[i] = thread(funcion_hebra_productora, i);
   }
   for(int i = 0; i < num_heb_consumidoras; i++){
      array_heb_consumidoras[i] = thread(funcion_hebra_consumidora, i);
   }

   for(int i = 0; i < num_heb_productoras; i++){
      array_heb_productoras[i].join();
   }
   for(int i = 0; i < num_heb_consumidoras; i++){
      array_heb_consumidoras[i].join();
   }

   test_contadores();
}
