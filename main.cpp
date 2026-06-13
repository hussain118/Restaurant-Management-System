#include "raylib.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>

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

    vector<string> getCartLines() const {
        vector<string> lines;
        if (itemCount == 0) {
            lines.push_back("Cart is empty!");
            return lines;
        }
        for (int i = 0; i < itemCount; i++) {
            lines.push_back(to_string(i + 1) + ". " + items[i].getDetails());
        }
        lines.push_back("Total: $" + to_string(totalAmount));
        return lines;
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
        : userId(id), name(n), email(e), password(p), role(r) {}

    virtual bool login(const string& e, const string& p) const {
        return (email == e && password == p);
    }

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
    string getPhone() const { return phone; }
    string getAddress() const { return address; }

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

    vector<string> getOrderLines() const {
        vector<string> lines;
        lines.push_back("Order ID: " + to_string(orderId) + " | Status: " + status + " | Total: $" + to_string(totalAmount));
        for (int i = 0; i < itemCount; i++) {
            lines.push_back(" - " + items[i].getDetails());
        }
        return lines;
    }
};
int Order::orderCounter = 0;

// ====== FILE MANAGER CLASS ======
class FileManager {
public:
    static bool adminFileEmpty() {
        ifstream file("admins.txt");
        return !file.good() || file.peek() == ifstream::traits_type::eof();
    }

    static void seedDefaultAdmin() {
        if (!adminFileEmpty()) return;
        Admin admin(1, "System Admin", "admin@food.com", "admin123", "ADMIN");
        ofstream out("admins.txt", ios::app);
        if (out.is_open()) {
            admin.saveToFile(out);
            out.close();
        }
    }

    static void loadProducts(Product* products[], int& count) {
        count = 0;
        ifstream file("products.txt");
        if (!file.is_open()) {
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
            return;
        }

        string line;
        while (getline(file, line) && count < MAX_PRODUCTS) {
            if (line.empty()) continue;
            string parts[6]; int pi = 0; string token;
            for (char ch : line) {
                if (ch == ',') { if (pi < 6) parts[pi++] = token; token = ""; }
                else token += ch;
            }
            if (pi < 6) parts[pi] = token;

            try {
                if (parts[0] == "FOOD") {
                    int id = stoi(parts[1]); float price = stof(parts[3]);
                    products[count++] = new FoodItem(id, parts[2], price, parts[4], parts[5]);
                }
            } catch (...) {}
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

    static int loadAdminsWithMaxId(Admin admins[], int maxCount, int& outMaxId) {
        outMaxId = 1; int count = 0;
        ifstream file("admins.txt"); if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            string parts[5]; int pi = 0; string token;
            for (char ch : line) {
                if (ch == ',') { if (pi < 5) parts[pi++] = token; token = ""; }
                else token += ch;
            }
            if (pi < 5) parts[pi] = token;
            try {
                int id = stoi(parts[0]);
                admins[count++] = Admin(id, parts[1], parts[2], parts[3], parts[4]);
                if (id > outMaxId) outMaxId = id;
            } catch (...) {}
        }
        file.close(); return count;
    }

    static int loadCustomers(Customer customers[], int maxCount) {
        int count = 0; ifstream file("customers.txt"); if (!file.is_open()) return 0;
        string line;
        while (getline(file, line) && count < maxCount) {
            if (line.empty()) continue;
            string parts[7]; int pi = 0; string token;
            for (char ch : line) {
                if (ch == ',') { if (pi < 7) parts[pi++] = token; token = ""; }
                else token += ch;
            }
            if (pi < 7) parts[pi] = token;
            try {
                int id = stoi(parts[0]);
                customers[count++] = Customer(id, parts[1], parts[2], parts[3], parts[4], parts[5], parts[6]);
                Customer::setCounter(id);
            } catch (...) {}
        }
        file.close(); return count;
    }
};

// ====== EXPOSED SYSTEM ENGINE ======
class OrderingSystem {
public:
    User* currentUser;
    Product* menu[MAX_PRODUCTS];
    int      menuCount;
    Admin    admins[MAX_ADMINS];
    int      adminCount;
    int      adminIdCounter;
    Customer customers[MAX_CUSTOMERS];
    int      customerCount;

    OrderingSystem() : currentUser(nullptr), menuCount(0), adminCount(0), adminIdCounter(1), customerCount(0) {
        FileManager::seedDefaultAdmin();
        FileManager::loadProducts(menu, menuCount);
        int maxAdminId = 1;
        adminCount = FileManager::loadAdminsWithMaxId(admins, MAX_ADMINS, maxAdminId);
        adminIdCounter = maxAdminId;
        customerCount = FileManager::loadCustomers(customers, MAX_CUSTOMERS);
    }
    ~OrderingSystem() { for (int i = 0; i < menuCount; i++) delete menu[i]; }

    Product* findProduct(int pid) {
        for (int i = 0; i < menuCount; i++)
            if (menu[i] != nullptr && menu[i]->getId() == pid) return menu[i];
        throw ProductNotFoundException(pid);
    }

    void registerCustomer(string n, string e, string p, string ph, string ad) {
        if (n.empty() || e.empty() || p.empty()) throw InvalidInputException("Missing core fields!");
        for (int i = 0; i < customerCount; i++) if (customers[i].getEmail() == e) throw InvalidInputException("Email exists!");
        int newId = Customer::getNextId();
        Customer c(newId, n, e, p, "CUSTOMER", ph, ad);
        customers[customerCount++] = c;
        ofstream file("customers.txt", ios::app);
        if (file.is_open()) { c.saveToFile(file); file.close(); }
    }

    bool attemptLogin(string e, string p, bool isAdmin) {
        if (isAdmin) {
            for (int i = 0; i < adminCount; i++) {
                if (admins[i].login(e, p)) { currentUser = &admins[i]; return true; }
            }
        } else {
            for (int i = 0; i < customerCount; i++) {
                if (customers[i].login(e, p)) { currentUser = &customers[i]; return true; }
            }
        }
        throw AuthException();
    }

    void addMenuProduct(string n, string cat, string exp, float p) {
        if (menuCount >= MAX_PRODUCTS) throw MenuFullException();
        int newId = 1;
        for (int i = 0; i < menuCount; i++) if (menu[i]->getId() >= newId) newId = menu[i]->getId() + 1;
        menu[menuCount++] = new FoodItem(newId, n, p, cat, exp);
        FileManager::saveAllProducts(menu, menuCount);
    }

    void removeMenuProduct(int pid) {
        for (int i = 0; i < menuCount; i++) {
            if (menu[i] != nullptr && menu[i]->getId() == pid) {
                delete menu[i];
                for (int j = i; j < menuCount - 1; j++) menu[j] = menu[j + 1];
                menu[menuCount - 1] = nullptr; menuCount--;
                FileManager::saveAllProducts(menu, menuCount);
                return;
            }
        }
        throw ProductNotFoundException(pid);
    }
};

// ====== RAYLIB GUI CONTROLLER ENGINE ======
enum ViewState { VIEW_LANDING, VIEW_CUST_LOGIN, VIEW_CUST_REGISTER, VIEW_ADMIN_LOGIN, VIEW_CUST_DASH, VIEW_ADMIN_DASH };

struct TerminalLogger {
    vector<string> backlogs;
    void log(const string& msg) {
        backlogs.push_back(">> " + msg);
        if (backlogs.size() > 14) backlogs.erase(backlogs.begin());
    }
    void draw(int x, int y, int width, int height) {
        DrawRectangle(x, y, width, height, GetColor(0x111116FF));
        DrawRectangleLines(x, y, width, height, GREEN);
        DrawText("SYSTEM TERMINAL CONSOLE ENGINE LOGS", x + 10, y + 10, 14, ORANGE);
        int currentY = y + 35;
        for (const auto& line : backlogs) {
            DrawText(line.c_str(), x + 15, currentY, 13, LIME);
            currentY += 20;
        }
    }
};

// Simple UI Elements Helper
bool DrawButton(Rectangle rect, const char* text, Color baseColor) {
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    DrawRectangleRec(rect, hovered ? ColorAlpha(baseColor, 0.8f) : baseColor);
    DrawRectangleLinesEx(rect, 2, WHITE);
    int textWidth = MeasureText(text, 16);
    DrawText(text, rect.x + (rect.width - textWidth)/2, rect.y + (rect.height - 16)/2, 16, WHITE);
    return (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
}

void DrawInputField(Rectangle rect, string& text, const char* label, bool active, int maxChars) {
    if (active) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (text.length() < maxChars)) text += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) text.pop_back();
    }
    DrawRectangleRec(rect, active ? GetColor(0x222233FF) : GetColor(0x1A1A1AFF));
    DrawRectangleLinesEx(rect, 1, active ? GREEN : GRAY);
    DrawText(text.c_str(), rect.x + 8, rect.y + 10, 16, active ? WHITE : LIGHTGRAY);
    DrawText(label, rect.x, rect.y - 18, 13, SKYBLUE);
    if (active && ((GetTime() - (int)GetTime()) > 0.5)) {
        int txtW = MeasureText(text.c_str(), 16);
        DrawRectangle(rect.x + 10 + txtW, rect.y + 8, 2, 20, GREEN);
    }
}

// ====== MAIN GRAPHICAL SYSTEM APPLICATION ======
int main() {
    const int screenWidth = 1100;
    const int screenHeight = 650;
    InitWindow(screenWidth, screenHeight, "FAST-NUCES Online Food Ordering System [Raylib Terminal Edition]");
    SetTargetFPS(60);

    OrderingSystem sys;
    TerminalLogger logger;
    logger.log("System Initialized successfully.");
    logger.log("Default Admin Account Configured: admin@food.com | pass: admin123");

    ViewState state = VIEW_LANDING;
    int activeField = 0;

    // Buffer Streams for inputs
    string txtEmail = "", txtPass = "";
    string regName = "", regEmail = "", regPass = "", regPhone = "", regAddr = "";
    string inputProdId = "", inputQty = "";
    string adminProdName = "", adminProdCat = "", adminProdExp = "", adminProdPrice = "";

    while (!WindowShouldClose()) {
        // --- LOGIC HANDLING UPDATE ---
        if (IsKeyPressed(KEY_TAB)) {
            activeField = (activeField + 1) % 5;
        }

        // --- GRAPHICS DRAWING FRAME ---
        BeginDrawing();
        ClearBackground(GetColor(0x0A0A0FFF));

        // Draw Frame Header
        DrawRectangle(0, 0, screenWidth, 50, GetColor(0x1F1F2EFF));
        DrawText("ADVANCED ONLINE FOOD ORDERING SYSTEM (GUI LAYER)", 20, 15, 20, GOLD);
        DrawLine(0, 50, screenWidth, 50, MAROON);

        // ALWAYS Display Console Logging Window at bottom
        logger.draw(20, 320, 1060, 310);

        // Main Interaction Window Space
        Rectangle mainWorkspace = { 20, 70, 1060, 235 };
        DrawRectangleRec(mainWorkspace, GetColor(0x16161FFF));
        DrawRectangleLinesEx(mainWorkspace, 1, DARKGRAY);

        switch (state) {
            case VIEW_LANDING: {
                DrawText("WELCOME TERMINAL PORTAL SELECTION", 50, 90, 18, SKYBLUE);
                if (DrawButton({ 50, 140, 200, 45 }, "CUSTOMER LOGIN", GetColor(0x34495EFF)))  { state = VIEW_CUST_LOGIN; activeField = 0; }
                if (DrawButton({ 270, 140, 200, 45 }, "CUSTOMER REGISTER", GetColor(0x27AE60FF))) { state = VIEW_CUST_REGISTER; activeField = 0; }
                if (DrawButton({ 490, 140, 200, 45 }, "ADMIN PORTAL", GetColor(0x7F8C8DFF)))     { state = VIEW_ADMIN_LOGIN; activeField = 0; }
                
                // Casual Browse directly on screen
                if (DrawButton({ 710, 140, 200, 45 }, "PRINT LIVE MENU", DARKGRAY)) {
                    logger.log("--- CURRENT RESTAURANT MENU ---");
                    for(int i=0; i<sys.menuCount; i++) logger.log(sys.menu[i]->getDetails());
                }
                break;
            }

            case VIEW_CUST_LOGIN: {
                DrawText("CUSTOMER AUTHENTICATION ACCESS", 40, 85, 16, GOLD);
                DrawInputField({ 40, 140, 250, 35 }, txtEmail, "Email Address", (activeField == 0), 30);
                DrawInputField({ 310, 140, 250, 35 }, txtPass, "Secret Password", (activeField == 1), 30);

                if (DrawButton({ 580, 140, 140, 35 }, "LOGIN", GREEN)) {
                    try {
                        if (sys.attemptLogin(txtEmail, txtPass, false)) {
                            logger.log("Success: Welcome back customer client (" + sys.currentUser->getName() + ")");
                            state = VIEW_CUST_DASH;
                        }
                    } catch(exception& e) { logger.log(string("Error Code: ") + e.what()); }
                }
                if (DrawButton({ 730, 140, 140, 35 }, "BACK", RED)) state = VIEW_LANDING;
                break;
            }

            case VIEW_CUST_REGISTER: {
                DrawText("CREATE CUSTOMER ACCOUNT PROFILE", 40, 80, 15, SKYBLUE);
                DrawInputField({ 40, 125, 160, 35 }, regName, "Name", (activeField == 0), 20);
                DrawInputField({ 210, 125, 180, 35 }, regEmail, "Email", (activeField == 1), 30);
                DrawInputField({ 400, 125, 150, 35 }, regPass, "Password", (activeField == 2), 20);
                DrawInputField({ 560, 125, 140, 35 }, regPhone, "Phone", (activeField == 3), 15);
                DrawInputField({ 710, 125, 170, 35 }, regAddr, "Address", (activeField == 4), 40);

                if (DrawButton({ 895, 125, 140, 35 }, "SUBMIT REG", GREEN)) {
                    try {
                        sys.registerCustomer(regName, regEmail, regPass, regPhone, regAddr);
                        logger.log("Success: Account registration complete. Try Logging in!");
                        state = VIEW_CUST_LOGIN;
                    } catch(exception& e) { logger.log(string("Error Code: ") + e.what()); }
                }
                if (DrawButton({ 895, 175, 140, 35 }, "BACK", RED)) state = VIEW_LANDING;
                break;
            }

            case VIEW_ADMIN_LOGIN: {
                DrawText("ADMIN PANEL AUTHENTICATION SYSTEM", 40, 85, 16, MAROON);
                DrawInputField({ 40, 140, 250, 35 }, txtEmail, "Admin Email ID", (activeField == 0), 30);
                DrawInputField({ 310, 140, 250, 35 }, txtPass, "Admin Code Pass", (activeField == 1), 30);

                if (DrawButton({ 580, 140, 140, 35 }, "ADMIN ACCESS", RED)) {
                    try {
                        if (sys.attemptLogin(txtEmail, txtPass, true)) {
                            logger.log("Success Authorized Account: " + sys.currentUser->getName());
                            state = VIEW_ADMIN_DASH;
                        }
                    } catch(exception& e) { logger.log(string("Error Code: ") + e.what()); }
                }
                if (DrawButton({ 730, 140, 140, 35 }, "BACK", DARKGRAY)) state = VIEW_LANDING;
                break;
            }

            case VIEW_CUST_DASH: {
                Customer* activeCust = dynamic_cast<Customer*>(sys.currentUser);
                DrawText((string("CUSTOMER OPERATIONS PROFILE: ") + activeCust->getName()).c_str(), 40, 80, 16, LIME);

                // Inputs to add items to cart
                DrawInputField({ 40, 130, 130, 35 }, inputProdId, "Item ID", (activeField == 0), 5);
                DrawInputField({ 180, 130, 100, 35 }, inputQty, "Qty", (activeField == 1), 5);

                if (DrawButton({ 290, 130, 120, 35 }, "ADD TO CART", BLUE)) {
                    try {
                        int pId = stoi(inputProdId); int qt = stoi(inputQty);
                        Product* p = sys.findProduct(pId);
                        activeCust->getCart().addItem(p, qt);
                        logger.log("Added: " + p->getName() + " x" + to_string(qt) + " to your active cart container.");
                    } catch(exception& e) { logger.log(string("Error: ") + e.what()); }
                }

                if (DrawButton({ 420, 130, 110, 35 }, "VIEW CART", GOLD)) {
                    logger.log("--- ACTIVE ONLINE BASKET ITEMS ---");
                    auto cartLines = activeCust->getCart().getCartLines();
                    for(const auto& l : cartLines) logger.log(l);
                }

                if (DrawButton({ 540, 130, 140, 35 }, "REMOVE PRODUCT", ORANGE)) {
                    try {
                        int pId = stoi(inputProdId);
                        activeCust->getCart().removeItem(pId);
                        logger.log("Removed Item ID: " + to_string(pId) + " from structural cart mapping.");
                    } catch(exception& e) { logger.log(string("Error: ") + e.what()); }
                }

                if (DrawButton({ 690, 130, 140, 35 }, "CHECKOUT / PLACE", GREEN)) {
                    if (activeCust->getCart().isEmpty()) {
                        logger.log("Error: Basket mapping array initialization missing elements!");
                    } else {
                        Order order(activeCust->getId(), activeCust->getCart().getItems(), activeCust->getCart().getItemCount(), activeCust->getCart().getTotal());
                        order.saveToFile();
                        logger.log("--- SYSTEM ISSUED GENERATED INVOICE BILL ---");
                        for(const auto& line : order.getOrderLines()) logger.log(line);
                        activeCust->getCart().clearCart();
                        logger.log("Success: Checkout file pipeline transaction processing finished safely.");
                    }
                }

                if (DrawButton({ 940, 130, 100, 35 }, "LOGOUT", RED)) {
                    sys.currentUser = nullptr;
                    logger.log("Logged out active transaction module wrapper context safely.");
                    state = VIEW_LANDING;
                }
                break;
            }

            case VIEW_ADMIN_DASH: {
                DrawText("ADMIN CONTROL PANEL OVERRIDE PLATFORM", 40, 75, 16, MAROON);

                DrawInputField({ 40, 120, 140, 35 }, adminProdName, "Item Name", (activeField == 0), 15);
                DrawInputField({ 190, 120, 110, 35 }, adminProdCat, "Category", (activeField == 1), 15);
                DrawInputField({ 310, 120, 110, 35 }, adminProdExp, "Expiry", (activeField == 2), 12);
                DrawInputField({ 430, 120, 90, 35 }, adminProdPrice, "Price", (activeField == 3), 6);

                if (DrawButton({ 535, 120, 100, 35 }, "ADD PRODUCT", LIME)) {
                    try {
                        float pr = stof(adminProdPrice);
                        sys.addMenuProduct(adminProdName, adminProdCat, adminProdExp, pr);
                        logger.log("Database Write: Added product (" + adminProdName + ") dynamically.");
                    } catch(exception& e) { logger.log(string("Error: ") + e.what()); }
                }

                // Reuse field 3 (Price box) as string holder for ID deletion tracking
                if (DrawButton({ 645, 120, 140, 35 }, "DELETE ITEM ID", RED)) {
                    try {
                        int pId = stoi(adminProdPrice);
                        sys.removeMenuProduct(pId);
                        logger.log("Database Update: Removed Target Product ID " + to_string(pId));
                    } catch(exception& e) { logger.log(string("Error: ") + e.what()); }
                }

                if (DrawButton({ 795, 120, 130, 35 }, "PRINT STATIONS", DARKGRAY)) {
                    logger.log("--- REGISTERED ADMIN INFRASTRUCTURE CONTROLLERS ---");
                    for (int i = 0; i < sys.adminCount; i++) {
                        logger.log(to_string(sys.admins[i].getId()) + " | " + sys.admins[i].getName() + " [" + sys.admins[i].getEmail() + "]");
                    }
                }

                if (DrawButton({ 940, 120, 100, 35 }, "LOGOUT", RED)) {
                    sys.currentUser = nullptr;
                    logger.log("Admin context pipeline tracking terminated cleanly.");
                    state = VIEW_LANDING;
                }
                
                DrawText("* Tip: To delete an item, enter its numeric ID inside the 'Price' frame box field and click Delete.", 40, 175, 13, GRAY);
                break;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}