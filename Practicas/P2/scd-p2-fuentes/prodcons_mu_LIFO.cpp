// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Practica 2. Introducción a los monitores en C++11.
//
// Archivo: prodcons_mu_LIFO.cpp
//
// Compilar: ~$ g++ -std=c++11 -pthread -I. -o prodcons_mu_LIFO prodcons_mu_LIFO.cpp scd.cpp
// -----------------------------------------------------------------------------------
/**
 * @file prodcons_mu_LIFO.cpp
 * @author Miguel Torres Alonso
 * @brief Ejemplo de un monitor en C++11 con semántica SU, para el problema del productor/consumidor, con productor y consumidor únicos.
 * @version 0.1
 * @date 2023-11-01
 * 
 */

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

constexpr int
   num_items = 1500 ;   // número de items a producir/consumir

const unsigned
   num_hebras_productoras = 5,
   num_hebras_consumidoras = 5;
   
constexpr int               
   min_ms    = 5,     // tiempo minimo de espera en sleep_for
   max_ms    = 20 ;   // tiempo máximo de espera en sleep_for

mutex
   mtx ;                 // mutex de escritura en pantalla

unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

// Array compartido con num_heb_productoras entradas que indique, en cada 
// momento, para cada hebra productora, cuántos items ha producido ya. Este array se 
// consulta y actualiza en producir_dato. Debe estar inicializado a 0.
int contador_productoras[num_hebras_productoras];

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato( unsigned id_hebra )
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   // Cada hebra produce valores entre el rango que le corresponde
   const unsigned dato_producido = id_hebra*(num_items/num_hebras_productoras) + contador_productoras[id_hebra]; 
   contador_productoras[id_hebra]++;
   mtx.lock();
   cout << "hebra productora numero " << id_hebra << ", produce " << dato_producido << endl << flush ;
   mtx.unlock();
   cont_prod[dato_producido]++ ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned valor_consumir , unsigned id_hebra)
{
   if ( num_items <= valor_consumir )
   {
      cout << " valor a consumir === " << valor_consumir << ", num_items == " << num_items << endl ;
      assert( valor_consumir < num_items );
   }
   cont_cons[valor_consumir] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<min_ms,max_ms>() ));
   mtx.lock();
   cout << "                  hebra consumidora numero " << id_hebra << ", consume: " << valor_consumir << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << endl ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, multiples prod/cons

class ProdConsMu : public HoareMonitor
{
 private:
 static const int           // constantes ('static' ya que no dependen de la instancia)
   num_celdas_total = 10;   //   núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//   buffer de tamaño fijo, con los datos
   primera_libre ;          //   indice de celda de la próxima inserción ( == número de celdas ocupadas)

 CondVar                    // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsMu() ;             // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsMu::ProdConsMu(  )
{
   primera_libre = 0 ;
   ocupadas      = newCondVar();
   libres        = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsMu::leer(  )
{
   // esperar bloqueado hasta que 0 < primera_libre
   if ( primera_libre == 0 )
      ocupadas.wait();

   //cout << "leer: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( 0 < primera_libre  );

   // hacer la operación de lectura, actualizando estado del monitor
   const int valor = buffer[--primera_libre] ;
   
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsMu::escribir( int valor )
{
   // esperar bloqueado hasta que primera_libre < num_celdas_total
   if ( primera_libre == num_celdas_total )
      libres.wait();

   //cout << "escribir: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre++] = valor ;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsMu> monitor, unsigned id_hebra )
{
   for( unsigned i = 0 ; i < num_items/num_hebras_productoras ; i++ )
   {
      int valor = producir_dato(id_hebra) ;
      monitor->escribir(valor);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsMu>  monitor , unsigned id_hebra)
{
   for( unsigned i = 0 ; i < num_items/num_hebras_consumidoras ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor , id_hebra) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema del productor-consumidor multiples (Monitor SU, buffer LIFO). " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<ProdConsMu> monitor = Create<ProdConsMu>() ;

   // Inicializacion del contador para que cada hebra productora produzca
   // dentro del rango [i*p, i*p+p-1]
   for(int i = 0; i < num_hebras_productoras; i++){
      contador_productoras[i] = 0;
   }

   // crear y lanzar las hebras
   thread array_hebras_productoras[num_hebras_productoras],
          array_hebras_consumidoras[num_hebras_consumidoras];

   for(int i = 0; i < num_hebras_productoras; i++){
      array_hebras_productoras[i] = thread(funcion_hebra_productora, monitor, i);
   }

   for(int i = 0; i < num_hebras_consumidoras; i++){
      array_hebras_consumidoras[i] = thread(funcion_hebra_consumidora, monitor, i);
   }

   // esperar a que terminen las hebras
   for(int i = 0; i < num_hebras_productoras; i++){
      array_hebras_productoras[i].join();
   }

   for(int i = 0; i < num_hebras_consumidoras; i++){
      array_hebras_consumidoras[i].join();
   }  

   test_contadores() ;
}
