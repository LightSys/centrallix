//NAME Native C ** SHOULD FAIL **

long long
test(char** name)
    {
    *name = "Native C ** SHOULD FAIL **";
    return -1;
    }
