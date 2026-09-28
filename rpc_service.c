// rpc_service.c: Implementación de las funciones RPC del servidor

#include <stdio.h> // printf
#include <stdlib.h> // malloc, free
#include <string.h> // memset, strdup
#include <signal.h> // signal, SIGINT, SIGTERM
#include <unistd.h> // write, STDOUT_FILENO
#include <rpc/rpc.h> // SVCXPRT, xdrproc_t, xdr_free

#include "claves.h" // Prototipos de la API local y struct Paquete
#include "clavesRPC.h" // Prototipos de las funciones RPC generadas por rpcgen (destroy_1_svc, set_value_1_svc, etc.)

#define MAX_STR 256 // 255 chars útiles + '\0'
#define MAX_V2 32 // Máximo número de elementos en V_value2 (N_value2 ∈ [1..32])

// Manejador de señal para terminar el servidor de forma controlada
static void cerrar_servidor(int sig) {
    (void)sig;

    const char msg[] = "\n[SERVIDOR] Cerrando servidor de forma segura...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    _exit(0);
}

__attribute__((constructor))
// Se ejecuta al iniciar el servidor para mostrar un mensaje de arranque y registrar señales
static void inicializar_servidor(void) {
    // Mensaje de arranque y registro de señales de parada
    printf("[SERVIDOR] Servidor escuchando peticiones RPC...\n");
    fflush(stdout);

    signal(SIGINT, cerrar_servidor);
    signal(SIGTERM, cerrar_servidor);
}

// Conversión de la estructura usada en RPC a la estructura local
static struct Paquete rpc_a_local_paquete(PaqueteRPC p) {
    struct Paquete out;
    out.x = p.x;
    out.y = p.y;
    out.z = p.z;
    return out;
}

// Conversión de la estructura local a la estructura serializable RPC
static PaqueteRPC local_a_rpc_paquete(struct Paquete p) {
    PaqueteRPC out;
    out.x = p.x;
    out.y = p.y;
    out.z = p.z;
    return out;
}

// destroy
bool_t destroy_1_svc(int *result, struct svc_req *rqstp) {
    (void)rqstp;

    printf("[SERVIDOR] PETICION: DESTROY\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    // Reenvía la operación a la implementación local
    *result = destroy();
    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// set_value
bool_t set_value_1_svc(ArgsSetModify arg1, int *result, struct svc_req *rqstp) {
    struct Paquete p; // Variable para almacenar el paquete convertido al formato local

    (void)rqstp;

    printf("[SERVIDOR] PETICION: SET_VALUE\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    // Convertimos value3 al formato de la API local antes de invocar set_value
    p = rpc_a_local_paquete(arg1.value3);

    // Reenvía la operación a la implementación local, pasando los argumentos convertidos
    *result = set_value(
        arg1.key,
        arg1.value1,
        arg1.N_value2,
        arg1.V_value2.V_value2_val,
        p
    );

    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// get_value
bool_t get_value_1_svc(char *arg1, GetValueResult *result, struct svc_req *rqstp) {
    char value1[MAX_STR]; // Buffer para almacenar value1 devuelto por la capa local
    int n_value2 = 0; // Variable para almacenar N_value2 devuelto por la capa local
    float v_value2[MAX_V2]; // Buffer para almacenar V_value2 devuelto por la capa local
    struct Paquete value3; // Variable para almacenar el paquete devuelto por la capa local

    (void)rqstp;

    printf("[SERVIDOR] PETICION: GET_VALUE\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    // Dejamos el resultado limpio para evitar basura en caso de error temprano
    memset(result, 0, sizeof(*result));

    if (arg1 == NULL) { // Si la clave es NULL -> error
        result->status = -1;
        return TRUE;
    }

    // Reenvía la operación a la implementación local, pasando los argumentos convertidos
    result->status = get_value(arg1, value1, &n_value2, v_value2, &value3);
    if (result->status != 0) { // Si la operación local indicó un error (status != 0) -> error
        return TRUE;
    }

    // Reservamos memoria dinámica porque XDR necesita buffers válidos para serializar
    result->value1 = strdup(value1);
    if (result->value1 == NULL) { // Si no se pudo reservar memoria para value1 -> error
        result->status = -1;
        return TRUE;
    }

    // Copiamos los valores de N_value2 y V_value2 al resultado RPC, reservando memoria dinámica para V_value2
    result->N_value2 = n_value2;
    result->V_value2.V_value2_len = n_value2;
    result->V_value2.V_value2_val = (float *)malloc(sizeof(float) * n_value2);

    if (result->V_value2.V_value2_val == NULL) { // Si no se pudo reservar memoria para V_value2 -> error
        free(result->value1);
        result->value1 = NULL;
        result->status = -1;
        return TRUE;
    }

    // Copiamos los valores de V_value2 al resultado RPC
    for (int i = 0; i < n_value2; i++) {
        result->V_value2.V_value2_val[i] = v_value2[i];
    }

    // Convertimos el paquete local al tipo RPC para devolverlo al cliente
    result->value3 = local_a_rpc_paquete(value3);

    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// modify_value
bool_t modify_value_1_svc(ArgsSetModify arg1, int *result, struct svc_req *rqstp) {
    struct Paquete p; // Variable para almacenar el paquete convertido al formato local

    (void)rqstp;

    printf("[SERVIDOR] PETICION: MODIFY_VALUE\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    // Convertimos value3 al formato interno y delegamos en la capa local
    p = rpc_a_local_paquete(arg1.value3);

    // Reenvía la operación a la implementación local, pasando los argumentos convertidos
    *result = modify_value(
        arg1.key,
        arg1.value1,
        arg1.N_value2,
        arg1.V_value2.V_value2_val,
        p
    );

    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// delete_key
bool_t delete_key_1_svc(char *arg1, int *result, struct svc_req *rqstp) {
    (void)rqstp;

    printf("[SERVIDOR] PETICION: DELETE_KEY\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    if (arg1 == NULL) { // Si la clave es NULL -> error
        *result = -1;
        return TRUE;
    }

    // Delegamos la eliminación en la implementación local
    *result = delete_key(arg1);
    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// exist
bool_t exist_1_svc(char *arg1, int *result, struct svc_req *rqstp) {
    (void)rqstp;

    printf("[SERVIDOR] PETICION: EXIST\n");
    fflush(stdout);

    if (result == NULL) return FALSE; // Si el puntero de resultado es NULL -> error

    if (arg1 == NULL) { // Si la clave es NULL -> error
        *result = -1;
        return TRUE;
    }

    // Reenvía la consulta de existencia a la capa local
    *result = exist(arg1);
    return TRUE; // Devuelve TRUE para indicar que se ha procesado la petición
}

// Función para liberar resultados dinámicos de llamadas RPC
int clavesrpc_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result) {
    (void)transp; // No necesitamos el transporte para liberar resultados

    // Libera memoria asociada al resultado dinámico de una llamada RPC
    if (result != NULL) { // Si el resultado no es NULL -> liberamos la memoria asociada usando la función XDR correspondiente
        xdr_free(xdr_result, result);
    }

    return 1; // Devuelve 1 para indicar que se ha liberado correctamente
}