// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2-mu.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
//
// -----------------------------------------------------------------------------

// Autor: Miguel Torres Alonso

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int num_productores = 4, num_consumidores = 5;
const int etiq_productor = 1, etiq_consumidor = 2;

const int
    id_buffer = num_productores,
    num_procesos_esperado = 10,
    num_items = 20,
    tam_vector = 10;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template <int min, int max>
int aleatorio()
{
   static default_random_engine generador((random_device())());
   static uniform_int_distribution<int> distribucion_uniforme(min, max);
   return distribucion_uniforme(generador);
}

// ---------------------------------------------------------------------
// producir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int rank)
{
   const int rango_valores = num_items / num_productores;
   static int contador = rank * rango_valores - 1; // -1 porque lo incrementamos ahora
   sleep_for(milliseconds(aleatorio<10, 100>()));
   contador++;
   cout << "Productor    " << rank << "   ha producido valor   " << contador << endl
        << flush;
   return contador;
}

// ---------------------------------------------------------------------

void funcion_productor(int rank)
{
   for (unsigned int i = 0; i < num_items / num_productores; i++)
   {
      // producir valor
      int valor_prod = producir(rank);

      // enviar valor
      cout << "Productor    " << rank << "   va a enviar  valor   " << valor_prod << endl
           << flush;
      MPI_Send(&valor_prod, 1, MPI_INT, id_buffer, etiq_productor, MPI_COMM_WORLD);
   }
}

// ---------------------------------------------------------------------

void consumir(int valor_cons, int rank)
{
   // espera bloqueada
   sleep_for(milliseconds(aleatorio<110, 200>()));
   cout << "Consumidor   " << rank << "   ha consumido valor   " << valor_cons << endl
        << flush;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int rank)
{
   int peticion,
       valor_rec = 1;
   MPI_Status estado;

   for (unsigned int i = 0; i < num_items / num_consumidores; i++)
   {
      // Enviamos peticion con etiq_consumidor para que el buffer sepa que es un consumidor
      MPI_Send(&peticion, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD);
      MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, etiq_consumidor, MPI_COMM_WORLD, &estado);

      cout << "Consumidor   " << rank << "   ha recibido  valor   " << valor_rec << endl
           << flush;
      consumir(valor_rec, rank);
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int buffer[tam_vector],      // buffer con celdas ocupadas y vacías
       valor,                   // valor recibido o enviado
       primera_libre = 0,       // índice de primera celda libre
       primera_ocupada = 0,     // índice de primera celda ocupada
       num_celdas_ocupadas = 0; // número de celdas ocupadas

   MPI_Status estado; // metadatos del mensaje recibido

   int etiq_aceptable;

   for (unsigned int i = 0; i < num_items * 2; i++)
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if (num_celdas_ocupadas == 0)               // si buffer vacío
         etiq_aceptable = etiq_productor;         // $~~~$ solo prod.
      else if (num_celdas_ocupadas == tam_vector) // si buffer lleno
         etiq_aceptable = etiq_consumidor;        // $~~~$ solo cons.
      else                                        // si no vacío ni lleno
         etiq_aceptable = MPI_ANY_TAG;            // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables
      MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);

      // 3. procesar el mensaje recibido

      switch (estado.MPI_TAG) // leer emisor del mensaje en metadatos
      {
      case etiq_productor: // si ha sido el productor: insertar en buffer
         buffer[primera_libre] = valor;
         primera_libre = (primera_libre + 1) % tam_vector;
         num_celdas_ocupadas++;
         cout << "Buffer ha recibido            valor   " << valor << endl;
         break;

      case etiq_consumidor: // si ha sido el consumidor: extraer y enviarle
         valor = buffer[primera_ocupada];
         primera_ocupada = (primera_ocupada + 1) % tam_vector;
         num_celdas_ocupadas--;
         // Vamos a enviar al mismo consumidor que ha solicitado
         int consum_aceptable = estado.MPI_SOURCE;
         cout << "Buffer va a enviar            valor   " << valor << endl;
         MPI_Send(&valor, 1, MPI_INT, consum_aceptable, etiq_consumidor, MPI_COMM_WORLD);
         break;
      }
   }
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{

   int id_propio, id_rol, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

   if (num_procesos_esperado == num_procesos_actual)
   {
      if (id_propio < num_productores)
      {
         id_rol = id_propio; // Los productores tienen el mismo id de rol que de comm
      }
      else if (id_propio > num_productores) // mayor estricto porque el id_propio == num_prod es el del buffer
      {
         id_rol = id_propio - num_productores - 1;
      }
      // ejecutar la operación apropiada a 'id_propio'
      if (id_propio < num_productores)
         funcion_productor(id_rol);
      else if (id_propio == id_buffer)
         funcion_buffer();
      else
         funcion_consumidor(id_rol); // Los que tengan id mayor que el buffer son consumidores respetando
                                     // el if(num_procesos_esperado == num_procesos_actual)
   }
   else
   {
      if (id_propio == 0) // solo el primero escribe error, indep. del rol
      {
         cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
              << "el número de procesos en ejecución es: " << num_procesos_actual << endl
              << "(programa abortado)" << endl;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize();
   return 0;
}
