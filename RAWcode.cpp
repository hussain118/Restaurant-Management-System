#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
using namespace std;

// ====== CONSTANTS ======
const int MAX_PRODUCTS   = 50;
const int MAX_CUSTOMERS  = 100;
const int MAX_ORDERS     = 100;
const int MAX_CART_ITEMS = 20;
const int MAX_ADMINS     = 10;

// ====== CUSTOM EXCEPTIONS ======
class CartFullException : public runtime_error {
public:
    CartFullException() : runtime_error("Cart is full! Maximum " + to_string(MAX_CART_ITEMS) + " items allowed.") {}
};

class MenuFullException : public runtime_error {
public:
    MenuFullException() : runtime_error("Menu is full! Maximum " + to_string(MAX_PRODUCTS) + " products allowed.") {}
};

class ProductNotFoundException : public runtime_error {
public:
    ProductNotFoundException(int id)
        : runtime_error("Product with ID " + to_string(id) + " not found.") {}
};

class InvalidInputException : public runtime_error {
public:
    InvalidInputException(const string& msg) : runtime_error("Invalid input: " + msg) {}
};

class FileException : public runtime_error {
public:
    FileException(const string& filename)
        : runtime_error("Could not open file: " + filename) {}
};

class AuthException : public runtime_error {
public:
    AuthException() : runtime_error("Invalid email or password.") {}
};

class AdminLimitException : public runtime_error {
public:
    AdminLimitException() : runtime_error("Admin limit reached! Maximum " + to_string(MAX_ADMINS) + " admins allowed.") {}
};

// ====== PRODUCT CLASSES ======
class Product {
protected:
    int    productId;
    string name;
    float  price;
    string category;
public:
    Product() : productId(0), price(0.0f) {}
    Product(int id, string n, float p, string c)
        : productId(id), name(n), price(p), category(c) {
        if (id <= 0)   throw InvalidInputException("Product ID must be positive.");
        if (n.empty()) throw InvalidInputException("Product name cannot be empty.");
        if (p < 0)     throw InvalidInputException("Price cannot be negative.");
    }

    virtual string getDetails() const {
        return to_string(productId) + " | " + name + " | $" + to_string(price);
    }

    int    getId()       const { return productId; }
    string getName()     const { return name; }
    float  getPrice()    const { return price; }
    string getCategory() const { return category; }

    virtual void saveToFile(ofstream& file) const {
        file << productId << "," << name << "," << price << "," << category << "\n";
    }

    virtual ~Product() {}
};

class FoodItem : public Product {
private:
    string expiryDate;
public:
    FoodItem() : Product() {}
    FoodItem(int id, string n, float p, string c, string exp)
        : Product(id, n, p, c), expiryDate(exp) {
        if (exp.empty()) throw InvalidInputException("Expiry date cannot be empty.");
    }

    string getDetails() const override {
        return Product::getDetails() + " | Expiry: " + expiryDate;
    }

    void saveToFile(ofstream& file) const override {
        file << "FOOD," << productId << "," << name << ","
             << price << "," << category << "," << expiryDate << "\n";
    }
};

// ====== ORDER ITEM CLASS ======
class OrderItem {
private:
    Product* product;
    int      quantity;
public:
    OrderItem() : product(nullptr), quantity(0) {}
    OrderItem(Product* p, int qty) : product(p), quantity(qty) {
        if (qty <= 0) throw InvalidInputException("Quantity must be at least 1.");
    }

    float    getSubTotal()  const { return (product == nullptr) ? 0 : product->getPrice() * quantity; }
    Product* getProduct()   const { return product; }
    int      getQuantity()  const { return quantity; }
    void     setQuantity(int qty) {
        if (qty <= 0) throw InvalidInputException("Quantity must be at least 1.");
        quantity = qty;
    }

    string getDetails() const {
        if (product == nullptr) return "";
        return product->getName() + " x" + to_string(quantity)
             + " = $" + to_string(getSubTotal());
    }
};

// ====== CART CLASS ======
class Cart {
private:
    int       customerId;
    OrderItem items[MAX_CART_ITEMS];
    int       itemCount;
    float     totalAmount;
public:
    Cart() : customerId(0), itemCount(0), totalAmount(0.0f) {}
    Cart(int cid) : customerId(cid), itemCount(0), totalAmount(0.0f) {}

    void addItem(Product* product, int qty) {
        if (product == nullptr) throw InvalidInputException("Cannot add a null product.");

        for (int i = 0; i < itemCount; i++) {
            if (items[i].getProduct() != nullptr &&
                items[i].getProduct()->getId() == product->getId()) {
                items[i].setQuantity(items[i].getQuantity() + qty);
                calculateTotal();
                return;
            }
        }

        if (itemCount >= MAX_CART_ITEMS) throw CartFullException();

        items[itemCount] = OrderItem(product, qty);
        itemCount++;
        calculateTotal();
    }

    void removeItem(int productId) {
        bool found = false;
        for (int i = 0; i < itemCount; i++) {
            if (items[i].getProduct() != nullptr &&
                items[i].getProduct()->getId() == productId) {
                for (int j = i; j < itemCount - 1; j++) items[j] = items[j + 1];
                itemCount--;
                found = true;
                break;
            }
        }
        if (!found) throw ProductNotFoundException(productId);
        calculateTotal();
    }

    void calculateTotal() {
        totalAmount = 0;
        for (int i = 0; i < itemCount; i++) totalAmount += items[i].getSubTotal();
    }

    void displayCart() const {
        cout << "\n--- YOUR CART ---" << endl;
        if (itemCount == 0) { cout << "Cart is empty!" << endl; return; }
        for (int i = 0; i < itemCount; i++)
            cout << i + 1 << ". " << items[i].getDetails() << endl;
        cout << "Total: $" << totalAmount << endl;
    }

    void clearCart() {
        itemCount   = 0;
        totalAmount = 0;
        for (int i = 0; i < MAX_CART_ITEMS; i++) items[i] = OrderItem();
    }

    OrderItem* getItems()     { return items; }
    int        getItemCount() const { return itemCount; }
    float      getTotal()     const { return totalAmount; }
    bool       isEmpty()      const { return itemCount == 0; }
};

// ====== USER CLASSES ======
class User {
protected:
    int    userId;
    string name, email, password, role;
public:
    User() : userId(0) {}
    User(int id, string n, string e, string p, string r)
        : userId(id), name(n), email(e), password(p), role(r) {
        if (e.empty()) throw InvalidInputException("Email cannot be empty.");
        if (p.empty()) throw InvalidInputException("Password cannot be empty.");
    }

    virtual bool login(const string& e, const string& p) const {
        return (email == e && password == p);
    }

    virtual void logout() const { cout << "Logged out successfully!" << endl; }

    string getRole()  const { return role; }
    int    getId()    const { return userId; }
    string getName()  const { return name; }
    string getEmail() const { return email; }

    virtual void saveToFile(ofstream& file) const = 0;
    virtual ~User() {}
};

class Customer : public User {
private:
    string phone, address;
    Cart   cart;

    static int customerCounter;
public:
    Customer() : User() {}
    Customer(int id, string n, string e, string p, string r, string ph, string addr)
        : User(id, n, e, p, r), phone(ph), address(addr), cart(id) {}

    static void setCounter(int val) { if (val > customerCounter) customerCounter = val; }
    static int  getNextId()         { return ++customerCounter; }

    Cart& getCart() { return cart; }

    void saveToFile(ofstream& file) const override {
        file << userId << "," << name << "," << email << "," << password << ","
             << role << "," << phone << "," << address << "\n";
    }
};
int Customer::customerCounter = 100;

class Admin : public User {
public:
    Admin() : User() {}
    Admin(int id, string n, string e, string p, string r)
        : User(id, n, e, p, r) {}

    void saveToFile(ofstream& file) const override {
        file << userId << "," << name << "," << email << ","
             << password << "," << role << "\n";
    }
};

// ====== ORDER CLASS ======
class Order {
private:
    static int orderCounter;
    int       orderId;
    int       customerId;
    OrderItem items[MAX_CART_ITEMS];
    int       itemCount;
    float     totalAmount;
    string    status;
public:
    Order() : orderId(0), customerId(0), itemCount(0), totalAmount(0), status("") {}
    Order(int cid, OrderItem cartItems[], int count, float total)
        : customerId(cid), itemCount(count), totalAmount(total) {
        if (count <= 0) throw InvalidInputException("Order must have at least one item.");
        orderId = ++orderCounter;
        status  = "Pending";
        for (int i = 0; i < count; i++) items[i] = cartItems[i];
    }

    void saveToFile() const {
        ofstream file("orders.txt", ios::app);
        if (!file.is_open()) throw FileException("orders.txt");
        file << orderId << "," << customerId << "," << totalAmount << "," << status << "\n";
        file.close();
    }

    void displayOrder() const {
        cout << "\nOrder ID: " << orderId
             << " | Status: " << status
             << " | Total: $" << totalAmount << endl;
        for (int i = 0; i < itemCount; i++)
            cout << " - " << items[i].getDetails() << endl;
    }
};
int Order::orderCounter = 0;

// ====== FILE MANAGER CLASS ======
class FileManager {
public:
    // Flush leftover characters from cin, then read one trimmed line
    static string readLine(const string& prompt) {
        string value;
        cout << prompt;
        // If there's a leftover newline in the buffer, skip it
        if (cin.peek() == '\n') cin.ignore();
        getline(cin, value);
        return value;
    }

    static int readInt(const string& prompt) {
        int value;
        cout << prompt;
        while (!(cin >> value)) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "[!] Expected a number. Try again: ";
        }
        cin.ignore(1000, '\n'); // consume trailing newline
        return value;
    }

    static float readFloat(const string& prompt) {
        float value;
        cout << prompt;
        while (!(cin >> value)) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "[!] Expected a number. Try again: ";
        }
        cin.ignore(1000, '\n');
        return value;
    }

    // Returns true if admins.txt is missing or empty
    static bool adminFileEmpty() {
        ifstream file("admins.txt");
        return !file.good() || file.peek() == ifstream::traits_type::eof();
    }

    // Create the very first admin (only called when file is empty)
    static void setupFirstAdmin() {
        if (!adminFileEmpty()) return; // already has admins — skip

        cout << "\n=== FIRST TIME ADMIN SETUP ===" << endl;
        string name     = readLine("Enter Admin Name: ");
        string email    = readLine("Enter Admin Email: ");
        string password = readLine("Enter Admin Password: ");

        if (name.empty() || email.empty() || password.empty())
            throw InvalidInputException("Admin name, email, and password cannot be empty.");

        Admin    admin(1, name, email, password, "ADMIN");
        ofstream out("admins.txt", ios::app);
        if (!out.is_open()) throw FileException("admins.txt");
        admin.saveToFile(out);
        out.close();
        cout << "\n[+] Default Admin Created Successfully!\n" << endl;
    }

    static void loadProducts(Product* products[], int& count) {
        count = 0;
        ifstream file("products.txt");
        if (!file.is_open()) {
            // Seed a default menu when no file exists yet
            products[count++] = new FoodItem(1, "Burger",      200.0f, "Food",     "2026-12-31");
            products[count++] = new FoodItem(2, "Pizza",       500.0f, "Food",     "2026-12-31");
            products[count++] = new FoodItem(3, "Cold Drink",  100.0f, "Beverage", "2026-12-31");
            products[count++] = new FoodItem(4, "French Fries", 150.0f, "Food",    "2026-12-31");
            products[count++] = new FoodItem(5, "Chicken Wrap", 350.0f, "Food",    "2026-12-31");

            ofstream out("products.txt");
            if (out.is_open()) {
                for (int i = 0; i < count; i++) products[i]->saveToFile(out);
                out.close();
            }
            cout << "[*] Default menu loaded (" << count << " items)." << endl;
            return;
        }

        string line;
        while (getline(file, line) && count < MAX_PRODUCTS) {
            if (line.empty()) continue;
            string parts[6];
            int    pi = 0;
            string token;
            for (char ch : line) {
                if (ch == ',') {
                    if (pi < 6) parts[pi++] = token;
                    token = "";
                } else {
                    token += ch;
                }
            }
            if (pi < 6) parts[pi] = token; // last field

            try {
                if (parts[0] == "FOOD") {
                    int   id    = stoi(parts[1]);
                    float price = stof(parts[3]);
                    products[count++] = new FoodItem(id, parts[2], price, parts[4], parts[5]);
                }
            } catch (...) {
                // Skip malformed lines silently
            }
        }
        file.close();
    }

    static void saveAllProducts(Product* products[], int count) {
        ofstream file("products.txt");
        if (!file.is_open()) throw FileException("products.txt");
        for (int i = 0; i < count; i++)
            if (products[i] != nullptr) products[i]->saveToFile(file);
        file.close();
    }

    // Returns the highest admin ID found in the file (to avoid ID collisions)
    static int loadAdmins(Admin admins[], int maxCount) {
        int count  = 0;
        int maxId  = 1;
        ifstream file("admins.txt");
        if (!file.is_open()) return 0;

        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            string parts[5];
            int    pi = 0;
            string token;
            for (char ch : line) {
                if (ch == ',') {
                    if (pi < 5) parts[pi++] = token;
                    token = "";
                } else {
                    token += ch;
                }
            }
            if (pi < 5) parts[pi] = token;

            try {
                int id = stoi(parts[0]);
                admins[count++] = Admin(id, parts[1], parts[2], parts[3], parts[4]);
                if (id > maxId) maxId = id;
            } catch (...) {}
        }
        file.close();
        return count; // caller sets adminIdCounter = maxId separately
    }

    // Also returns the max ID seen so the system can avoid collisions
    static int loadAdminsWithMaxId(Admin admins[], int maxCount, int& outMaxId) {
        outMaxId   = 1;
        int count  = 0;
        ifstream file("admins.txt");
        if (!file.is_open()) return 0;

        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            string parts[5];
            int    pi = 0;
            string token;
            for (char ch : line) {
                if (ch == ',') {
                    if (pi < 5) parts[pi++] = token;
                    token = "";
                } else {
                    token += ch;
                }
            }
            if (pi < 5) parts[pi] = token;

            try {
                int id = stoi(parts[0]);
                admins[count++] = Admin(id, parts[1], parts[2], parts[3], parts[4]);
                if (id > outMaxId) outMaxId = id;
            } catch (...) {}
        }
        file.close();
        return count;
    }

    static int loadCustomers(Customer customers[], int maxCount) {
        int count = 0;
        ifstream file("customers.txt");
        if (!file.is_open()) return 0;

        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            string parts[7];
            int    pi = 0;
            string token;
            for (char ch : line) {
                if (ch == ',') {
                    if (pi < 7) parts[pi++] = token;
                    token = "";
                } else {
                    token += ch;
                }
            }
            if (pi < 7) parts[pi] = token;

            try {
                int id = stoi(parts[0]);
                customers[count++] = Customer(id, parts[1], parts[2],
                                              parts[3], parts[4], parts[5], parts[6]);
                Customer::setCounter(id); // keep static counter ahead of file IDs
            } catch (...) {}
        }
        file.close();
        return count;
    }
};

// ====== MAIN SYSTEM CLASS ======
class OrderingSystem {
private:
    User*    currentUser;
    Product* menu[MAX_PRODUCTS];
    int      menuCount;

    Admin    admins[MAX_ADMINS];
    int      adminCount;
    int      adminIdCounter; // tracks highest admin ID for new admin creation

    Customer customers[MAX_CUSTOMERS];
    int      customerCount;

public:
    OrderingSystem()
        : currentUser(nullptr), menuCount(0),
          adminCount(0), adminIdCounter(1), customerCount(0)
    {
        FileManager::loadProducts(menu, menuCount);

        // Load admins and capture the highest ID used so far
        int maxAdminId = 1;
        adminCount = FileManager::loadAdminsWithMaxId(admins, MAX_ADMINS, maxAdminId);
        adminIdCounter = maxAdminId; // new admins will get IDs > this

        customerCount = FileManager::loadCustomers(customers, MAX_CUSTOMERS);
    }

    ~OrderingSystem() {
        for (int i = 0; i < menuCount; i++) delete menu[i];
    }

    // ---- helpers ----
    Product* findProduct(int pid) {
        for (int i = 0; i < menuCount; i++)
            if (menu[i] != nullptr && menu[i]->getId() == pid)
                return menu[i];
        throw ProductNotFoundException(pid);
    }

    // ---- landing page ----
    void showLandingPage() {
        int choice = 0;
        do {
            cout << "\n╔══════════════════════════╗" << endl;
            cout << "║   FOOD ORDERING SYSTEM   ║" << endl;
            cout << "╠══════════════════════════╣" << endl;
            cout << "║  [1] CUSTOMER LOGIN      ║" << endl;
            cout << "║  [2] CUSTOMER REGISTER   ║" << endl;
            cout << "║  [3] ADMIN LOGIN         ║" << endl;
            cout << "║  [4] BROWSE MENU         ║" << endl;
            cout << "║  [5] EXIT                ║" << endl;
            cout << "╚══════════════════════════╝" << endl;

            try {
                choice = FileManager::readInt("Select Option: ");
                switch (choice) {
                    case 1: customerLogin();    break;
                    case 2: customerRegister(); break;
                    case 3: adminLogin();       break;
                    case 4: displayMenu();      break;
                    case 5: cout << "Thank you! Goodbye." << endl; break;
                    default: cout << "Invalid option! Enter 1-5." << endl;
                }
            } catch (const AuthException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const InvalidInputException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const exception& e) {
                cout << "\n[Error] " << e.what() << endl;
            }
        } while (choice != 5);
    }

    // ---- auth ----
    void customerRegister() {
        cout << "\n--- CUSTOMER REGISTRATION ---" << endl;
        string name     = FileManager::readLine("Enter Name: ");
        string email    = FileManager::readLine("Enter Email: ");
        string password = FileManager::readLine("Enter Password: ");
        string phone    = FileManager::readLine("Enter Phone: ");
        string address  = FileManager::readLine("Enter Address: ");

        if (name.empty() || email.empty() || password.empty())
            throw InvalidInputException("Name, email, and password are required.");

        for (int i = 0; i < customerCount; i++)
            if (customers[i].getEmail() == email)
                throw InvalidInputException("An account with this email already exists.");

        if (customerCount >= MAX_CUSTOMERS)
            throw runtime_error("Customer limit reached.");

        int      newId = Customer::getNextId();
        Customer c(newId, name, email, password, "CUSTOMER", phone, address);
        customers[customerCount++] = c;

        ofstream file("customers.txt", ios::app);
        if (!file.is_open()) throw FileException("customers.txt");
        c.saveToFile(file);
        file.close();

        cout << "\n[+] Account Created Successfully! Please Login.\n" << endl;
    }

    void customerLogin() {
        cout << "\n--- CUSTOMER LOGIN ---" << endl;
        string email    = FileManager::readLine("Enter Email: ");
        string password = FileManager::readLine("Enter Password: ");

        for (int i = 0; i < customerCount; i++) {
            if (customers[i].login(email, password)) {
                currentUser = &customers[i];
                cout << "\n[+] Login Successful! Welcome, " << currentUser->getName() << endl;
                customerDashboard();
                currentUser = nullptr;
                return;
            }
        }
        throw AuthException();
    }

    void adminLogin() {
        cout << "\n--- ADMIN LOGIN ---" << endl;
        string email    = FileManager::readLine("Enter Email: ");
        string password = FileManager::readLine("Enter Password: ");

        for (int i = 0; i < adminCount; i++) {
            if (admins[i].login(email, password)) {
                currentUser = &admins[i];
                cout << "\n[+] Admin Login Successful! Welcome, " << currentUser->getName() << endl;
                adminDashboard();
                currentUser = nullptr;
                return;
            }
        }
        throw AuthException();
    }

    // ---- menu display ----
    void displayMenu() const {
        cout << "\n========== MENU ==========" << endl;
        if (menuCount == 0) { cout << "No items on menu yet." << endl; return; }
        for (int i = 0; i < menuCount; i++)
            if (menu[i] != nullptr)
                cout << menu[i]->getDetails() << endl;
        cout << "==========================" << endl;
    }

    // ---- customer dashboard ----
    void customerDashboard() {
        Customer* cust = dynamic_cast<Customer*>(currentUser);
        if (!cust) return;

        int choice = 0;
        do {
            cout << "\n╔══════════════════════════╗" << endl;
            cout << "║  Welcome, " << currentUser->getName() << endl;
            cout << "╠══════════════════════════╣" << endl;
            cout << "║  [1] VIEW MENU & ADD     ║" << endl;
            cout << "║  [2] VIEW CART           ║" << endl;
            cout << "║  [3] REMOVE CART ITEM    ║" << endl;
            cout << "║  [4] PLACE ORDER         ║" << endl;
            cout << "║  [5] LOGOUT              ║" << endl;
            cout << "╚══════════════════════════╝" << endl;

            try {
                choice = FileManager::readInt("Select Option: ");
                switch (choice) {
                    case 1: displayMenu(); addToCart(cust);   break;
                    case 2: cust->getCart().displayCart();     break;
                    case 3: removeFromCart(cust);             break;
                    case 4: placeOrder(cust);                 break;
                    case 5: currentUser->logout();             break;
                    default: cout << "Invalid option!" << endl;
                }
            } catch (const CartFullException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const ProductNotFoundException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const InvalidInputException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const exception& e) {
                cout << "\n[Error] " << e.what() << endl;
            }
        } while (choice != 5);
    }

    void addToCart(Customer* cust) {
        int pid = FileManager::readInt("Enter Product ID to add: ");
        int qty = FileManager::readInt("Enter Quantity: ");
        if (qty <= 0) throw InvalidInputException("Quantity must be at least 1.");

        Product* p = findProduct(pid);
        cust->getCart().addItem(p, qty);
        cout << "[+] " << p->getName() << " x" << qty << " added to cart!" << endl;
    }

    void removeFromCart(Customer* cust) {
        cust->getCart().displayCart();
        if (cust->getCart().isEmpty()) return;
        int pid = FileManager::readInt("Enter Product ID to remove: ");
        cust->getCart().removeItem(pid);
        cout << "[+] Item removed from cart." << endl;
    }

    void placeOrder(Customer* cust) {
        if (cust->getCart().isEmpty()) {
            cout << "Cart is empty! Add items first." << endl;
            return;
        }
        Order order(cust->getId(),
                    cust->getCart().getItems(),
                    cust->getCart().getItemCount(),
                    cust->getCart().getTotal());
        order.saveToFile();
        order.displayOrder();
        cust->getCart().clearCart();
        cout << "\n[+] Order Placed Successfully!" << endl;
    }

    // ---- admin dashboard ----
    void adminDashboard() {
        int choice = 0;
        do {
            cout << "\n╔══════════════════════════╗" << endl;
            cout << "║      ADMIN PANEL         ║" << endl;
            cout << "╠══════════════════════════╣" << endl;
            cout << "║  [1] VIEW MENU           ║" << endl;
            cout << "║  [2] ADD MENU ITEM       ║" << endl;
            cout << "║  [3] REMOVE MENU ITEM    ║" << endl;
            cout << "║  [4] CREATE NEW ADMIN    ║" << endl;
            cout << "║  [5] LIST ALL ADMINS     ║" << endl;
            cout << "║  [6] LOGOUT              ║" << endl;
            cout << "╚══════════════════════════╝" << endl;

            try {
                choice = FileManager::readInt("Select Option: ");
                switch (choice) {
                    case 1: displayMenu();          break;
                    case 2: adminAddMenuItem();     break;
                    case 3: adminRemoveMenuItem();  break;
                    case 4: adminCreateAdmin();     break;
                    case 5: adminListAdmins();      break;
                    case 6: currentUser->logout();  break;
                    default: cout << "Invalid option!" << endl;
                }
            } catch (const MenuFullException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const AdminLimitException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const ProductNotFoundException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const InvalidInputException& e) {
                cout << "\n[!] " << e.what() << endl;
            } catch (const FileException& e) {
                cout << "\n[File Error] " << e.what() << endl;
            } catch (const exception& e) {
                cout << "\n[Error] " << e.what() << endl;
            }
        } while (choice != 6);
    }

    void adminAddMenuItem() {
        if (menuCount >= MAX_PRODUCTS) throw MenuFullException();

        cout << "\n--- ADD MENU ITEM ---" << endl;

        // Auto-generate a product ID greater than all existing ones
        int newId = 1;
        for (int i = 0; i < menuCount; i++)
            if (menu[i] != nullptr && menu[i]->getId() >= newId)
                newId = menu[i]->getId() + 1;

        string name     = FileManager::readLine("Enter Item Name: ");
        string category = FileManager::readLine("Enter Category: ");
        string expiry   = FileManager::readLine("Enter Expiry Date (YYYY-MM-DD): ");
        float  price    = FileManager::readFloat("Enter Price: $");

        if (name.empty() || category.empty() || expiry.empty())
            throw InvalidInputException("Name, category, and expiry date cannot be empty.");
        if (price < 0)
            throw InvalidInputException("Price cannot be negative.");

        menu[menuCount++] = new FoodItem(newId, name, price, category, expiry);
        FileManager::saveAllProducts(menu, menuCount);

        cout << "\n[+] Item \"" << name << "\" added to menu with ID " << newId << "." << endl;
    }

    void adminRemoveMenuItem() {
        displayMenu();
        if (menuCount == 0) return;

        int pid = FileManager::readInt("Enter Product ID to remove: ");

        for (int i = 0; i < menuCount; i++) {
            if (menu[i] != nullptr && menu[i]->getId() == pid) {
                string removedName = menu[i]->getName();
                delete menu[i];
                for (int j = i; j < menuCount - 1; j++) menu[j] = menu[j + 1];
                menu[menuCount - 1] = nullptr;
                menuCount--;
                FileManager::saveAllProducts(menu, menuCount);
                cout << "\n[+] Item \"" << removedName << "\" removed from menu." << endl;
                return;
            }
        }
        throw ProductNotFoundException(pid);
    }

    void adminCreateAdmin() {
        if (adminCount >= MAX_ADMINS) throw AdminLimitException();

        cout << "\n--- CREATE NEW ADMIN ---" << endl;
        string name     = FileManager::readLine("Enter Admin Name: ");
        string email    = FileManager::readLine("Enter Admin Email: ");
        string password = FileManager::readLine("Enter Admin Password: ");

        if (name.empty() || email.empty() || password.empty())
            throw InvalidInputException("Name, email, and password cannot be empty.");

        for (int i = 0; i < adminCount; i++)
            if (admins[i].getEmail() == email)
                throw InvalidInputException("An admin with this email already exists.");

        int   newId    = ++adminIdCounter;
        Admin newAdmin(newId, name, email, password, "ADMIN");
        admins[adminCount++] = newAdmin;

        ofstream file("admins.txt", ios::app);
        if (!file.is_open()) throw FileException("admins.txt");
        newAdmin.saveToFile(file);
        file.close();

        cout << "\n[+] New admin \"" << name
             << "\" created successfully (ID: " << newId << ")." << endl;
    }

    void adminListAdmins() const {
        cout << "\n--- ADMIN LIST ---" << endl;
        if (adminCount == 0) { cout << "No admins found." << endl; return; }
        for (int i = 0; i < adminCount; i++)
            cout << admins[i].getId() << " | "
                 << admins[i].getName()  << " | "
                 << admins[i].getEmail() << endl;
    }
};

// ====== MAIN ======
int main() {
    try {
        // Only runs when admins.txt is missing or empty — creates the one-time first admin
        FileManager::setupFirstAdmin();

        OrderingSystem system;
        system.showLandingPage();

    } catch (const FileException& e) {
        cout << "\n[Fatal File Error] " << e.what() << endl;
        return 1;
    } catch (const InvalidInputException& e) {
        cout << "\n[Fatal Input Error] " << e.what() << endl;
        return 1;
    } catch (const exception& e) {
        cout << "\n[Fatal Error] " << e.what() << endl;
        return 1;
    }
    return 0;
}