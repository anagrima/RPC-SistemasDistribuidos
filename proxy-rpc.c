// proxy-rpc.c: Implementación de la API local 

#include <stdio.h> // printf
#include <stdlib.h> // malloc, free
#include <string.h> // strncpy, strnlen
#include <rpc/rpc.h> // Para CLIENT, clnt_create, clnt_destroy, clnt_perror, enum clnt_stat

#include "claves.h" // Prototipos de la API local y struct Paquete
#include "clavesRPC.h" // Prototipos de las funciones RPC generadas por rpcgen (set_value_1, get_value_1, etc.)

#define MAX_STR 256 // 255 chars útiles + '\0'
#define MAX_V2 32 // Máximo número de elementos en V_value2 (N_value2 ∈ [1..32])

// Crea un cliente RPC usando la IP definida en la variable de entorno IP_TUPLAS
static CLIENT *crear_cliente_rpc(void) {
    char *ip = getenv("IP_TUPLAS"); // Se espera que esta variable esté definida en el entorno del proceso

    if (ip == NULL || *ip == '\0') { // Si la variable no está definida o es una cadena vacía -> error
        fprintf(stderr, "Error: la variable de entorno IP_TUPLAS no esta definida.\n");
        return NULL;
    }

    // Creamos el cliente RPC para comunicarnos con el servidor
    CLIENT *clnt = clnt_create(ip, CLAVESRPC_PROG, CLAVESRPC_VERS, "tcp");
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        clnt_pcreateerror(ip);
        return NULL;
    }

    return clnt; // Cliente creado exitosamente
}

// Convierte un paquete RPC a un paquete local (para pasar al servidor)
static void copiar_paquete_rpc_a_local(struct Paquete *dst, PaqueteRPC src) {
    // Copia campo a campo para pasar del tipo RPC al tipo de la API local
    dst->x = src.x;
    dst->y = src.y;
    dst->z = src.z;
}

// Convierte un paquete local a un paquete RPC (para enviar al servidor)
static PaqueteRPC copiar_paquete_local_a_rpc(struct Paquete src) {
    // Copia campo a campo para pasar del tipo local al tipo serializable RPC
    PaqueteRPC p;
    p.x = src.x;
    p.y = src.y;
    p.z = src.z;
    return p; // Devuelve el paquete convertido al formato RPC
}

// destroy
int destroy(void) {
    CLIENT *clnt = crear_cliente_rpc(); // Cliente RPC para comunicarnos con el servidor
    int result = -1; // Variable para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    // Llamada remota DESTROY
    estado = destroy_1(&result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC destroy fallo");
        clnt_destroy(clnt);
        return -1;
    }

    clnt_destroy(clnt); // Destruimos el cliente RPC para liberar recursos
    return result; // Devolvemos el resultado de la llamada remota
}

// set_value
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    CLIENT *clnt; // Cliente RPC para comunicarnos con el servidor
    ArgsSetModify args; // Estructura para empaquetar los argumentos de la llamada remota
    int result = -1; // Variable para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (key == NULL || value1 == NULL || V_value2 == NULL) { // Si alguno de los argumentos es NULL -> error
        return -1;
    }

    clnt = crear_cliente_rpc(); // Creamos el cliente RPC para comunicarnos con el servidor
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    // Empaquetamos argumentos locales en la estructura RPC
    memset(&args, 0, sizeof(args));
    args.key = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.V_value2_len = N_value2;
    args.V_value2.V_value2_val = V_value2;
    args.value3 = copiar_paquete_local_a_rpc(value3);

    // Llamada remota SET_VALUE
    estado = set_value_1(args, &result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC set_value fallo");
        clnt_destroy(clnt);
        return -1;
    }

    clnt_destroy(clnt); // Destruimos el cliente RPC para liberar recursos
    return result; // Devolvemos el resultado de la llamada remota
}

// get_value
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    CLIENT *clnt; // Cliente RPC para comunicarnos con el servidor
    GetValueResult result; // Estructura para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (key == NULL || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        // Si alguno de los argumentos es NULL -> error
        return -1;
    }

    // Creamos el cliente RPC para comunicarnos con el servidor
    clnt = crear_cliente_rpc();
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    memset(&result, 0, sizeof(result)); // Limpiamos la estructura de resultado para evitar basura en caso de error temprano

    // Llamada remota GET_VALUE
    estado = get_value_1(key, &result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC get_value fallo");
        clnt_destroy(clnt);
        return -1;
    }

    if (result.status != 0) { // Si el servidor indicó un error (status != 0) -> error
        // Liberamos cualquier memoria asociada al resultado RPC antes de salir
        xdr_free((xdrproc_t)xdr_GetValueResult, (char *)&result);
        clnt_destroy(clnt);
        return -1;
    }

    strncpy(value1, result.value1, MAX_STR - 1); // Copiamos value1 al buffer local (asume buffer >=256)
    value1[MAX_STR - 1] = '\0'; // Aseguramos que value1 esté null-terminated

    *N_value2 = result.N_value2; // Copiamos N_value2 al buffer local
    if (*N_value2 < 1 || *N_value2 > MAX_V2) { // Si N_value2 no está en el rango válido -> error
        xdr_free((xdrproc_t)xdr_GetValueResult, (char *)&result);
        clnt_destroy(clnt);
        return -1;
    }

    if ((int)result.V_value2.V_value2_len != *N_value2) { // Si la longitud de V_value2 no coincide con N_value2 -> error
        xdr_free((xdrproc_t)xdr_GetValueResult, (char *)&result);
        clnt_destroy(clnt);
        return -1;
    }

    // Copiamos los valores de V_value2 al buffer local (asume buffer >=32)
    for (int i = 0; i < *N_value2; i++) {
        V_value2[i] = result.V_value2.V_value2_val[i];
    }

    // Limpiamos el resto de V_value2 para evitar basura en campos no usados
    for (int i = *N_value2; i < MAX_V2; i++) {
        V_value2[i] = 0.0f;
    }

    // Copiamos el paquete devuelto por RPC al formato de la API local
    copiar_paquete_rpc_a_local(value3, result.value3);

    // Liberación final de buffers dinámicos generados por XDR
    xdr_free((xdrproc_t)xdr_GetValueResult, (char *)&result);
    clnt_destroy(clnt);
    return 0; // Éxito
}

// modify_value
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    CLIENT *clnt; // Cliente RPC para comunicarnos con el servidor
    ArgsSetModify args; // Estructura para empaquetar los argumentos de la llamada remota
    int result = -1; // Variable para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (key == NULL || value1 == NULL || V_value2 == NULL) { // Si alguno de los argumentos es NULL -> error
        return -1;
    }

    // Creamos el cliente RPC para comunicarnos con el servidor
    clnt = crear_cliente_rpc();
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    // Empaquetamos argumentos para la llamada remota MODIFY_VALUE
    memset(&args, 0, sizeof(args));
    args.key = key;
    args.value1 = value1;
    args.N_value2 = N_value2;
    args.V_value2.V_value2_len = N_value2;
    args.V_value2.V_value2_val = V_value2;
    args.value3 = copiar_paquete_local_a_rpc(value3);

    // Llamada remota MODIFY_VALUE
    estado = modify_value_1(args, &result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC modify_value fallo");
        clnt_destroy(clnt);
        return -1;
    }

    clnt_destroy(clnt); // Destruimos el cliente RPC para liberar recursos
    return result; // Devolvemos el resultado de la llamada remota
}

// delete_key
int delete_key(char *key) {
    CLIENT *clnt; // Cliente RPC para comunicarnos con el servidor
    int result = -1; // Variable para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (key == NULL) { // Si la clave es NULL -> error
        return -1;
    }

    // Creamos el cliente RPC para comunicarnos con el servidor
    clnt = crear_cliente_rpc();
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    // Llamada remota DELETE_KEY
    estado = delete_key_1(key, &result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC delete_key fallo");
        clnt_destroy(clnt);
        return -1;
    }

    clnt_destroy(clnt); // Destruimos el cliente RPC para liberar recursos
    return result; // Devolvemos el resultado de la llamada remota
}

// exist
int exist(char *key) {
    CLIENT *clnt; // Cliente RPC para comunicarnos con el servidor
    int result = -1; // Variable para almacenar el resultado de la llamada remota
    enum clnt_stat estado; // Variable para almacenar el estado de la llamada remota

    if (key == NULL) { // Si la clave es NULL -> error
        return -1;
    }

    clnt = crear_cliente_rpc(); // Creamos el cliente RPC para comunicarnos con el servidor
    if (clnt == NULL) { // Si no se pudo crear el cliente -> error
        return -1;
    }

    // Llamada remota EXIST
    estado = exist_1(key, &result, clnt);
    if (estado != RPC_SUCCESS) { // Si la llamada remota falló -> error
        clnt_perror(clnt, "RPC exist fallo");
        clnt_destroy(clnt);
        return -1;
    }

    clnt_destroy(clnt); // Destruimos el cliente RPC para liberar recursos
    return result; // Devolvemos el resultado de la llamada remota
}