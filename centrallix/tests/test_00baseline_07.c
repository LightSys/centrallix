//NAME Native C ** SHOULD LOCKUP **

long long
test(char** name)
    {
    *name = "Native C ** SHOULD LOCKUP **";
    while(1);
    return 0;
    }
