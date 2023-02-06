#include <stdio.h>

static int bank_balance = 1000;

int deposit(int amount, int account_balance) 
{
    bank_balance += amount;
    return (account_balance + amount);
}

int main()
{
    int tom_balance = 100;
    int bob_balance = 200;
                
    // receive paychecks
    tom_balance = deposit(500, tom_balance);
    bob_balance = deposit(500, bob_balance);
    printf("Tom : %d\n",tom_balance);
    printf("Bob : %d\n",bob_balance);
    printf("Bank: %d\n",bank_balance);
    
    return 0;
}