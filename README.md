A desktop food ordering application built for my Object-Oriented Programming course at FAST NUCES, Karachi, with a custom graphical interface rendered in raylib. Users can register, browse a categorized menu, manage a cart, place orders, and generate an itemized bill.

Built the entire GUI on raylib from graphics primitives — custom buttons, menus, input handling, and screen-state management — instead of a prebuilt widget toolkit
Applied core OOP fundamentals: inheritance (a User base specialized into Customer and Admin), polymorphism (uniform handling of different user and menu-item types), and encapsulation (private state behind clean interfaces)
Used operator overloading for cart and billing operations and STL containers for menu/cart management
Implemented file handling to persist user and order data across sessions, with exception handling for invalid input
Core classes: User, MenuItem, Cart, Order, Payment, Admin

