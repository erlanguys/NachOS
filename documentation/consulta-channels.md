Creemos que tenemos un problema de inanicion en nuestro codigo para la implementacion de la clase Channel (nosotros la hemos llamado Port), nuestra implementacion es la siguiente:

```cpp
void
Port::Send(int message)
{
    // esperamos tener "turno"
    lockPort->Acquire();
    // esperamos que haya adónde enviar el mensaje
    while(not bufferPointer)
        canSend->Wait();
    // enviamos el mensaje
    *bufferPointer = message;
    bufferPointer = nullptr;
    // avisamos que se puede recibir de nuevo
    canReceive->Signal();  // <<< (Zona 3)
    // avisamos que terminamos de enviar el mensaje
    doneSending->Signal();  // <<< (Zona 4)
    // liberamos el lock
    lockPort->Release();
}

void
Port::Receive(int *destination)
{
    // esperar turno
    lockPort->Acquire();
    // esperar que esté libre el buffer
    while( bufferPointer )
        canReceive->Wait(); // <<< (Zona 1)
    // asignar el buffer
    bufferPointer = destination;
    // avisar que se puede escribir
    canSend->Signal();
    // aguardamos hasta que se termine de escribir
    while( bufferPointer )
        doneSending->Wait();  // <<< (Zona 2)
    // liberamos el lock
    lockPort->Release();
}
```

Nuestras variables privadas son estas:

```c++
    const char *name;
    int *bufferPointer;
    Lock *lockPort;

    Condition *doneSending;
    Condition *canSend;
    Condition *canReceive;
```

El problema ocurre cuando dos threads usan `Port::Receive`, y luego llega uno que usa `Port::Send`, la traza seria la siguiente.

`Thread1` llama a `Port::Receive`, como `bufferPointer` es nulo, lo llena y queda esperando en Zona 2.
`Thread2` llama a `Port::Receive`, ahora `bufferPointer` esta lleno, por lo que queda esperando en Zona 1.
`Thread3` llama a `Port:Send`, escribe la informacion en el buffer de `Thread1`, libera `bufferPointer`, y señaliza `canReceive` y `doneSending`.
`Thread2` se despierta por haber recibido la señal de `canReceive`, como `bufferPointer` ya es null, y `Thread2` puede continuar y escribe `bufferPointer`. Luego, queda esperando en Zona 2.
`Thread1` se despierta y como el `bufferPointer` está usado, no abandona más la Zona 2. Esto se repite ad infinitum.

Solución propuesta: inmediatamente arriba de Zona 2, cambiar la condición del while por `bufferPointer == destination`.

Es correcta esta solución?