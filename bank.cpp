#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sqlite3.h>

using namespace std;

class BankAccount
{
private:
    string accountNumber;
    int pin;
    double balance;

public:
    BankAccount(string accNum, int p, double bal) : accountNumber(accNum), pin(p), balance(bal) {}

    string getAccountNumber() const { return accountNumber; }
    int getPin() const { return pin; }
    bool verifyPin(int inputPin) const { return pin == inputPin; }
    double getBalance() const { return balance; }
    void deposit(double amount) { balance += amount; }
    bool withdraw(double amount)
    {
        if (amount <= balance)
        {
            balance -= amount;
            return true;
        }
        return false;
    }
};

sqlite3 *db;
char *errMsg = 0;

void initDatabase()
{
    int rc = sqlite3_open("bank.db", &db);
    if (rc)
    {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        exit(0);
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS accounts ("
                      "account_number TEXT PRIMARY KEY,"
                      "pin INTEGER,"
                      "balance REAL);";

    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

string generateAccountNumber()
{
    string accNum = "";
    for (int i = 0; i < 10; i++)
    {
        accNum += to_string(rand() % 10);
    }
    return accNum;
}

void saveAccount(const BankAccount &account)
{
    string sql = "INSERT INTO accounts (account_number, pin, balance) VALUES ('" +
                 account.getAccountNumber() + "', " +
                 to_string(account.getPin()) + ", " +
                 to_string(account.getBalance()) + ");";

    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

BankAccount *findAccount(const string &accountNumber)
{
    string sql = "SELECT * FROM accounts WHERE account_number = '" + accountNumber + "';";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return nullptr;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        string accNum = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        int pin = sqlite3_column_int(stmt, 1);
        double balance = sqlite3_column_double(stmt, 2);
        sqlite3_finalize(stmt);
        return new BankAccount(accNum, pin, balance);
    }

    sqlite3_finalize(stmt);
    return nullptr;
}

void updateAccountBalance(const BankAccount &account)
{
    string sql = "UPDATE accounts SET balance = " + to_string(account.getBalance()) +
                 " WHERE account_number = '" + account.getAccountNumber() + "';";

    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

int main()
{
    srand(time(0));
    initDatabase();

    int choice, pin;
    string accountNumber;
    double amount;
    BankAccount *account = nullptr;

    while (true)
    {
        cout << "\nBanking System Menu:\n";
        cout << "1. Create Account\n";
        cout << "2. Deposit\n";
        cout << "3. Withdraw\n";
        cout << "4. Check Balance\n";
        cout << "5. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
        case 1:
            accountNumber = generateAccountNumber();
            cout << "Enter PIN for your new account: ";
            cin >> pin;
            account = new BankAccount(accountNumber, pin, 0);
            saveAccount(*account);
            cout << "Account created successfully. Your account number is: " << accountNumber << endl;
            break;
        case 2:
            cout << "Enter account number: ";
            cin >> accountNumber;
            account = findAccount(accountNumber);
            if (account)
            {
                cout << "Enter PIN: ";
                cin >> pin;
                if (account->verifyPin(pin))
                {
                    cout << "Enter amount to deposit: ";
                    cin >> amount;
                    account->deposit(amount);
                    updateAccountBalance(*account);
                    cout << "Deposit successful. New balance: $" << fixed << setprecision(2) << account->getBalance() << endl;
                }
                else
                {
                    cout << "Incorrect PIN.\n";
                }
            }
            else
            {
                cout << "Account not found.\n";
            }
            break;
        case 3:
            cout << "Enter account number: ";
            cin >> accountNumber;
            account = findAccount(accountNumber);
            if (account)
            {
                cout << "Enter PIN: ";
                cin >> pin;
                if (account->verifyPin(pin))
                {
                    cout << "Enter amount to withdraw: ";
                    cin >> amount;
                    if (account->withdraw(amount))
                    {
                        updateAccountBalance(*account);
                        cout << "Withdrawal successful. New balance: $" << fixed << setprecision(2) << account->getBalance() << endl;
                    }
                    else
                    {
                        cout << "Insufficient funds.\n";
                    }
                }
                else
                {
                    cout << "Incorrect PIN.\n";
                }
            }
            else
            {
                cout << "Account not found.\n";
            }
            break;
        case 4:
            cout << "Enter account number: ";
            cin >> accountNumber;
            account = findAccount(accountNumber);
            if (account)
            {
                cout << "Enter PIN: ";
                cin >> pin;
                if (account->verifyPin(pin))
                {
                    cout << "Current balance: $" << fixed << setprecision(2) << account->getBalance() << endl;
                }
                else
                {
                    cout << "Incorrect PIN.\n";
                }
            }
            else
            {
                cout << "Account not found.\n";
            }
            break;
        case 5:
            cout << "Thank you for using our banking system. Goodbye!\n";
            sqlite3_close(db);
            return 0;
        default:
            cout << "Invalid choice. Please try again.\n";
        }

        delete account;
        account = nullptr;
    }

    return 0;
}