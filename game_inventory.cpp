#include <iostream>
#include <string>
#include <limits>

// ============================================================
// Game Inventory Management System using Singly Linked List
// CSD2183 Data Structures - Assignment 1
// TUT-T3 Group 01
//
// Description:
//   A console-based inventory manager for a game. Items are
//   stored in a Singly Linked List. Supports adding, removing,
//   searching, updating and sorting items via Merge Sort.
//
// Menu options:
//   1 - Display all items in the inventory
//   2 - Add a new item (name, rarity, value, quantity)
//   3 - Remove an item by its ID
//   4 - Search for an item by name
//   5 - Update the quantity of an item by its ID
//   6 - Sort inventory (by name, value, rarity, or quantity)
//   7 - Exit the program
//
// The inventory comes pre-loaded with 6 sample items.
// ============================================================


// Enum for item rarity
enum class Rarity { COMMON, UNCOMMON, RARE, EPIC, LEGENDARY };

// Convert rarity enum to string
std::string rarityToString(Rarity r) {
    switch (r) {
        case Rarity::COMMON:    return "Common";
        case Rarity::UNCOMMON:  return "Uncommon";
        case Rarity::RARE:      return "Rare";
        case Rarity::EPIC:      return "Epic";
        case Rarity::LEGENDARY: return "Legendary";
        default:                return "Unknown";
    }
}

// Convert integer input to Rarity
Rarity intToRarity(int val) {
    switch (val) {
        case 0: return Rarity::COMMON;
        case 1: return Rarity::UNCOMMON;
        case 2: return Rarity::RARE;
        case 3: return Rarity::EPIC;
        case 4: return Rarity::LEGENDARY;
        default: return Rarity::COMMON;
    }
}

// Get rarity rank for sorting (higher = rarer)
int rarityRank(Rarity r) {
    switch (r) {
        case Rarity::COMMON:    return 0;
        case Rarity::UNCOMMON:  return 1;
        case Rarity::RARE:      return 2;
        case Rarity::EPIC:      return 3;
        case Rarity::LEGENDARY: return 4;
        default:                return 0;
    }
}

// ============================================================
// Node structure for the Singly Linked List
// Each node represents one inventory item
// ============================================================
struct Item {
    int    itemID;
    std::string itemName;
    Rarity rarity;
    int    value;
    int    quantity;
    Item*  next;

    Item(int id, const std::string& name, Rarity r, int val, int qty)
        : itemID(id), itemName(name), rarity(r), value(val), quantity(qty), next(nullptr) {}
};

// ============================================================
// Inventory class – Singly Linked List with sorting
// ============================================================
class Inventory {
private:
    Item* head;
    int   nextID;   // auto-incrementing ID

    // --- Merge Sort helpers (O(n log n)) ---

    // Split list into two halves using slow/fast pointer technique
    void splitList(Item* source, Item** front, Item** back) {
        Item* slow = source;
        Item* fast = source->next;
        while (fast != nullptr) {
            fast = fast->next;
            if (fast != nullptr) {
                slow = slow->next;
                fast = fast->next;
            }
        }
        *front = source;
        *back  = slow->next;
        slow->next = nullptr;
    }

    // Merge two sorted lists by a chosen criterion
    // mode: 0 = by name, 1 = by value, 2 = by rarity
    Item* sortedMerge(Item* a, Item* b, int mode) {
        if (!a) return b;
        if (!b) return a;

        Item* result = nullptr;
        bool aFirst = false;

        switch (mode) {
            case 0: // name (alphabetical)
                aFirst = (a->itemName <= b->itemName);
                break;
            case 1: // value (ascending)
                aFirst = (a->value <= b->value);
                break;
            case 2: // rarity (ascending rank)
                aFirst = (rarityRank(a->rarity) <= rarityRank(b->rarity));
                break;
            case 3: // quantity (ascending)
                aFirst = (a->quantity <= b->quantity);
                break;
        }

        if (aFirst) {
            result = a;
            result->next = sortedMerge(a->next, b, mode);
        } else {
            result = b;
            result->next = sortedMerge(a, b->next, mode);
        }
        return result;
    }

    // Recursive merge sort
    void mergeSort(Item** headRef, int mode) {
        Item* h = *headRef;
        if (!h || !h->next) return; // base case: 0 or 1 node

        Item* a = nullptr;
        Item* b = nullptr;
        splitList(h, &a, &b);

        mergeSort(&a, mode);
        mergeSort(&b, mode);

        *headRef = sortedMerge(a, b, mode);
    }

public:
    Inventory() : head(nullptr), nextID(1) {}

    ~Inventory() {
        Item* current = head;
        while (current) {
            Item* temp = current;
            current = current->next;
            delete temp;
        }
    }

    // --- Core SLL operations ---

    // Add item at head – O(1)
    void addItem(const std::string& name, Rarity rarity, int value, int quantity) {
        Item* newItem = new Item(nextID++, name, rarity, value, quantity);
        newItem->next = head;
        head = newItem;
        std::cout << "Added: " << name << " (ID " << newItem->itemID << ")\n";
    }

    // Remove item by ID – O(n)
    bool removeItem(int id) {
        Item* current = head;
        Item* prev = nullptr;

        while (current != nullptr) {
            if (current->itemID == id) {
                if (prev == nullptr) {
                    head = current->next;  // removing head
                } else {
                    prev->next = current->next;
                }
                std::cout << "Removed: " << current->itemName << " (ID " << id << ")\n";
                delete current;
                return true;
            }
            prev = current;
            current = current->next;
        }
        std::cout << "Item with ID " << id << " not found.\n";
        return false;
    }

    // Search by name – O(n)
    Item* searchByName(const std::string& name) {
        Item* current = head;
        while (current != nullptr) {
            if (current->itemName == name) return current;
            current = current->next;
        }
        return nullptr;
    }

    // Search by ID – O(n)
    Item* searchByID(int id) {
        Item* current = head;
        while (current != nullptr) {
            if (current->itemID == id) return current;
            current = current->next;
        }
        return nullptr;
    }

    // Update item quantity – O(n) search + O(1) update
    bool updateQuantity(int id, int newQty) {
        Item* item = searchByID(id);
        if (item) {
            item->quantity = newQty;
            std::cout << "Updated " << item->itemName << " quantity to " << newQty << ".\n";
            if (newQty <= 0) {
                std::cout << "Quantity is 0 or less — removing item.\n";
                removeItem(id);
            }
            return true;
        }
        std::cout << "Item with ID " << id << " not found.\n";
        return false;
    }

    // Sort inventory – O(n log n) using merge sort
    void sortInventory(int mode) {
        if (!head || !head->next) {
            std::cout << "Inventory has fewer than 2 items, no sorting needed.\n";
            return;
        }
        mergeSort(&head, mode);
        std::string criteria;
        switch (mode) {
            case 0: criteria = "Name";   break;
            case 1: criteria = "Value";  break;
            case 2: criteria = "Rarity"; break;
            case 3: criteria = "Quantity"; break;
        }
        std::cout << "Inventory sorted by " << criteria << ".\n";
    }

    // Display all items – O(n) traversal
    void displayInventory() const {
        if (!head) {
            std::cout << "\nInventory is empty.\n";
            return;
        }

        std::cout << "\n===========================================================\n";
        std::cout << "                   GAME INVENTORY                          \n";
        std::cout << "===========================================================\n";
        std::cout << "ID   | Name                 | Rarity     | Value | Qty\n";
        std::cout << "-----------------------------------------------------------\n";

        Item* current = head;
        int count = 0;
        int totalValue = 0;
        while (current != nullptr) {
            printf("%-4d | %-20s | %-10s | %-5d | %d\n",
                   current->itemID,
                   current->itemName.c_str(),
                   rarityToString(current->rarity).c_str(),
                   current->value,
                   current->quantity);
            totalValue += current->value * current->quantity;
            count++;
            current = current->next;
        }
        std::cout << "-----------------------------------------------------------\n";
        std::cout << "Total items: " << count << "  |  Total inventory value: " << totalValue << "\n";
        std::cout << "===========================================================\n";
    }

    // Count items – O(n) traversal
    int countItems() const {
        int count = 0;
        Item* current = head;
        while (current) { count++; current = current->next; }
        return count;
    }
};

// ============================================================
// Helper: safe integer input
// ============================================================
int getIntInput(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input. Please enter a number.\n";
    }
}

std::string getStringInput(const std::string& prompt) {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    return s;
}

// ============================================================
// Main – interactive menu
// ============================================================
int main() {
    Inventory inv;

    // Pre-populate with sample items for demonstration
    inv.addItem("Iron Sword",    Rarity::COMMON,    50,  1);
    inv.addItem("Health Potion",  Rarity::COMMON,    25,  5);
    inv.addItem("Dragon Shield",  Rarity::EPIC,     500,  1);
    inv.addItem("Phoenix Feather",Rarity::LEGENDARY,1000, 1);
    inv.addItem("Steel Helmet",   Rarity::UNCOMMON, 120,  1);
    inv.addItem("Mana Elixir",    Rarity::RARE,     200,  3);

    std::cout << "\n=== Game Inventory Management System ===\n";
    std::cout << "Using Singly Linked List with Merge Sort\n";

    bool running = true;
    while (running) {
        std::cout << "\n--- Menu ---\n";
        std::cout << "1. Display Inventory\n";
        std::cout << "2. Add Item\n";
        std::cout << "3. Remove Item (by ID)\n";
        std::cout << "4. Search Item (by Name)\n";
        std::cout << "5. Update Quantity\n";
        std::cout << "6. Sort Inventory\n";
        std::cout << "7. Exit\n";

        int choice = getIntInput("Enter choice: ");

        switch (choice) {
        case 1:
            inv.displayInventory();
            break;

        case 2: {
            std::string name = getStringInput("Item name: ");
            std::cout << "Rarity (0=Common, 1=Uncommon, 2=Rare, 3=Epic, 4=Legendary): ";
            int r = getIntInput("");
            int val = getIntInput("Value: ");
            int qty = getIntInput("Quantity: ");
            inv.addItem(name, intToRarity(r), val, qty);
            break;
        }

        case 3: {
            int id = getIntInput("Enter item ID to remove: ");
            inv.removeItem(id);
            break;
        }

        case 4: {
            std::string name = getStringInput("Enter item name to search: ");
            Item* found = inv.searchByName(name);
            if (found) {
                std::cout << "Found: " << found->itemName
                          << " | Rarity: " << rarityToString(found->rarity)
                          << " | Value: " << found->value
                          << " | Qty: " << found->quantity << "\n";
            } else {
                std::cout << "Item not found.\n";
            }
            break;
        }

        case 5: {
            int id  = getIntInput("Enter item ID: ");
            int qty = getIntInput("New quantity: ");
            inv.updateQuantity(id, qty);
            break;
        }

        case 6: {
            std::cout << "Sort by: 0=Name, 1=Value, 2=Rarity, 3=Quantity\n";
            int mode = getIntInput("Choice: ");
            if (mode >= 0 && mode <= 3) {
                inv.sortInventory(mode);
                inv.displayInventory();
            } else {
                std::cout << "Invalid sort option.\n";
            }
            break;
        }

        case 7:
            running = false;
            std::cout << "Exiting. Goodbye!\n";
            break;

        default:
            std::cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}
