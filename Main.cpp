#include <iostream>
#include <string>
#include <ctime>
#include <algorithm>
using namespace std;

struct Book {
    string title;
    string author;
    string ISBN;
    bool available;
    Book* next;
};

struct User {
    int id;
    string name;
    string contact;
    string password;
    User* next;
};

struct Transaction {
    string ISBN;
    int userID;
    string action;
    string time;
    Transaction* next;
};

struct StackNode {
    string ISBN;
    int userID;
    string action;
    StackNode* next;
};

struct QueueNode {
    int userID;
    string ISBN;
    QueueNode* next;
};

Book* bookHead = NULL;
User* userHead = NULL;
Transaction* transactionHead = NULL;
StackNode* undoStack = NULL;
QueueNode* borrowQueueFront = NULL;
QueueNode* borrowQueueRear = NULL;

string currentTime() {
    time_t now = time(0);
    char* dt = ctime(&now);
    return string(dt);
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string trim(string s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == string::npos || end == string::npos) return "";
    return s.substr(start, end - start + 1);
}

void addBook(string title, string author, string ISBN) {
    Book* newBook = new Book{trim(title), trim(author), trim(ISBN), true, NULL};
    if (!bookHead) bookHead = newBook;
    else {
        Book* temp = bookHead;
        while (temp->next) temp = temp->next;
        temp->next = newBook;
    }
    cout << "Book added successfully!\n";
}

void displayBooks(Book* head) {
    if (!head) { cout << "No books.\n"; return; }
    cout << "\n------ Books ------\n";
    while (head) {
        cout << "Title: " << head->title << "\nAuthor: " << head->author
             << "\nISBN: " << head->ISBN << "\nAvailable: "
             << (head->available ? "Yes" : "No") << "\n\n";
        head = head->next;
    }
}

Book* searchBookByTitle(Book* head, string title) {
    if (!head) return NULL;
    if (toLower(trim(head->title)) == toLower(trim(title))) return head;
    return searchBookByTitle(head->next, title);
}

Book* searchBookByISBN(Book* head, string isbn) {
    while (head) {
        if (trim(head->ISBN) == trim(isbn)) return head;
        head = head->next;
    }
    return NULL;
}

void addUser(int id, string name, string contact, string password) {
    User* newUser = new User{id, trim(name), trim(contact), trim(password), NULL};
    if (!userHead) userHead = newUser;
    else {
        User* temp = userHead;
        while (temp->next) temp = temp->next;
        temp->next = newUser;
    }
    cout << "User added successfully!\n";
}

User* searchUserByID(User* head, int id) {
    while (head) {
        if (head->id == id) return head;
        head = head->next;
    }
    return NULL;
}

User* authenticateUser(int id, string password) {
    User* u = searchUserByID(userHead, id);
    if (u && u->password == password) return u;
    return NULL;
}

void addTransaction(string ISBN, int userID, string action) {
    Transaction* newTran = new Transaction{trim(ISBN), userID, action, currentTime(), NULL};
    if (!transactionHead) transactionHead = newTran;
    else {
        Transaction* temp = transactionHead;
        while (temp->next) temp = temp->next;
        temp->next = newTran;
    }

    StackNode* newStack = new StackNode{trim(ISBN), userID, action, undoStack};
    undoStack = newStack;
}

void displayTransactions(int filterUserID = -1) {
    if (!transactionHead) { cout << "No transactions.\n"; return; }
    Transaction* temp = transactionHead;
    cout << "\n--- Transaction History ---\n";
    while (temp) {
        if (filterUserID == -1 || temp->userID == filterUserID) {
            cout << "UserID: " << temp->userID << ", ISBN: " << temp->ISBN
                 << ", Action: " << temp->action << ", Time: " << temp->time;
        }
        temp = temp->next;
    }
}

void undoLastTransaction() {
    if (!undoStack) {
        cout << "No transaction to undo!\n";
        return;
    }
    StackNode* top = undoStack;
    Book* book = searchBookByISBN(bookHead, top->ISBN);

    if (book) {
        if (top->action == "borrowed") book->available = true;
        else if (top->action == "returned") book->available = false;
    }

    Transaction* prev = NULL;
    Transaction* temp = transactionHead;
    while (temp->next) { prev = temp; temp = temp->next; }
    if (prev) { prev->next = NULL; delete temp; }
    else { delete transactionHead; transactionHead = NULL; }

    undoStack = undoStack->next;
    delete top;

    cout << "Last transaction undone!\n";
}

void enqueueBorrow(int userID, string ISBN) {
    QueueNode* newNode = new QueueNode{userID, trim(ISBN), NULL};
    if (!borrowQueueFront) borrowQueueFront = borrowQueueRear = newNode;
    else {
        borrowQueueRear->next = newNode;
        borrowQueueRear = newNode;
    }
    cout << "Added to pending borrow queue.\n";
}

void removeQueueNode(QueueNode* prev, QueueNode* node) {
    if (!prev) {
        borrowQueueFront = node->next;
        if (!borrowQueueFront) borrowQueueRear = NULL;
    } else {
        prev->next = node->next;
        if (!prev->next) borrowQueueRear = prev;
    }
    delete node;
}

void borrowBook(int userID, string title) {
    User* user = searchUserByID(userHead, userID);
    Book* book = searchBookByTitle(bookHead, title);

    if (!user) { cout << "User not found!\n"; return; }
    if (!book) { cout << "Book not found!\n"; return; }
    if (!book->available) {
        cout << "Book already borrowed. Added to queue.\n";
        enqueueBorrow(userID, book->ISBN);
        return;
    }

    book->available = false;
    addTransaction(book->ISBN, userID, "borrowed");
    cout << "Book borrowed successfully!\n";
}

void returnBook(int userID, string title) {
    User* user = searchUserByID(userHead, userID);
    Book* book = searchBookByTitle(bookHead, title);

    if (!user || !book) { cout << "Invalid user or book!\n"; return; }
    if (book->available) { cout << "Book was not borrowed!\n"; return; }

    book->available = true;
    addTransaction(book->ISBN, userID, "returned");
    cout << "Book returned successfully!\n";

    QueueNode* temp = borrowQueueFront;
    QueueNode* prev = NULL;
    while (temp) {
        if (temp->ISBN == book->ISBN) {
            borrowBook(temp->userID, book->title);
            removeQueueNode(prev, temp);
            break;
        }
        prev = temp;
        temp = temp->next;
    }
}

const string ADMIN_PASSWORD = "admin123";

void mainMenu() {
    cout << "\n==============================\n";
    cout << "      LIBRARY MANAGEMENT       \n";
    cout << "==============================\n";
    cout << "1. Admin Login\n2. User Login\n3. Exit\n";
    cout << "Choice: ";
}

void adminMenu() {
    while (true) {
        cout << "\n-----------------------------\n";
        cout << "          ADMIN MENU         \n";
        cout << "-----------------------------\n";
        cout << "1. Add Book\n2. Add User\n3. Display All Books\n4. Display All Transactions\n5. Undo Last Transaction\n0. Logout\nChoice: ";
        int c; cin >> c; cin.ignore();
        if (c == 0) break;
        if (c == 1) {
            string t,a,isbn;
            cout << "Title: "; getline(cin,t);
            cout << "Author: "; getline(cin,a);
            cout << "ISBN: "; getline(cin,isbn);
            addBook(t,a,isbn);
        } else if (c == 2) {
            int id; string name, contact, pass;
            cout << "User ID: "; cin >> id; cin.ignore();
            cout << "Name: "; getline(cin,name);
            cout << "Contact: "; getline(cin,contact);
            cout << "Password: "; getline(cin,pass);
            addUser(id,name,contact,pass);
        } else if (c == 3) {
            displayBooks(bookHead);
        } else if (c == 4) {
            displayTransactions();
        } else if (c == 5) {
            undoLastTransaction();
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

void userMenu(User* u) {
    while (true) {
        cout << "\n-----------------------------\n";
        cout << "         USER MENU           \n";
        cout << "-----------------------------\n";
        cout << "1. View Books\n2. Borrow Book\n3. Return Book\n4. View My Transactions\n0. Logout\nChoice: ";
        int c; cin >> c; cin.ignore();
        if (c == 0) break;
        if (c == 1) displayBooks(bookHead);
        else if (c == 2) {
            string title; cout << "Book Title: "; getline(cin,title);
            borrowBook(u->id,title);
        } else if (c == 3) {
            string title; cout << "Book Title: "; getline(cin,title);
            returnBook(u->id,title);
        } else if (c == 4) displayTransactions(u->id);
        else cout << "Invalid choice.\n";
    }
}

int main() {
    while (true) {
        mainMenu();
        int choice; cin >> choice; cin.ignore();
        if (choice == 3) break;

        if (choice == 1) {
            string pass; cout << "Enter admin password: "; getline(cin,pass);
            if (pass == ADMIN_PASSWORD) adminMenu();
            else cout << "Wrong admin password!\n";
        } else if (choice == 2) {
            int id; string pass;
            cout << "User ID: "; cin >> id; cin.ignore();
            cout << "Password: "; getline(cin,pass);
            User* u = authenticateUser(id,pass);
            if (!u) cout << "Login failed: wrong ID or password.\n";
            else userMenu(u);
        } else {
            cout << "Invalid choice.\n";
        }
    }
    cout << "Goodbye!\n";
}