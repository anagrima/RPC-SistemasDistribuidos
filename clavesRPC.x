struct PaqueteRPC {
    int x;
    int y;
    int z;
};

struct ArgsSetModify {
    string key<256>;
    string value1<256>;
    int N_value2;
    float V_value2<32>;
    PaqueteRPC value3;
};

struct GetValueResult {
    int status;
    string value1<256>;
    int N_value2;
    float V_value2<32>;
    PaqueteRPC value3;
};

program CLAVESRPC_PROG {
    version CLAVESRPC_VERS {
        int DESTROY(void) = 1;
        int SET_VALUE(ArgsSetModify) = 2;
        GetValueResult GET_VALUE(string) = 3;
        int MODIFY_VALUE(ArgsSetModify) = 4;
        int DELETE_KEY(string) = 5;
        int EXIST(string) = 6;
    } = 1;
} = 0x20495785;