================================================================
   GAME INVENTORY MANAGEMENT SYSTEM - USER GUIDE
   CSD2183 Data Structures - Assignment 1
   TUT-T3 Group 01
================================================================


 1. OVERVIEW
----------------------------------------------------------------
 This is a console-based Game Inventory Management System
 built in C++. Items are stored using a Singly Linked List
 data structure. The system supports adding, removing,
 searching, updating and sorting items using Merge Sort.


 2. HOW TO COMPILE
----------------------------------------------------------------
 Using g++ (MinGW / MSYS2 / Linux):

   g++ -o game_inventory game_inventory.cpp -std=c++17

 Using Visual Studio Developer Command Prompt:

   cl /EHsc /std:c++17 game_inventory.cpp


 3. HOW TO RUN
----------------------------------------------------------------
 Windows:    game_inventory.exe
 Linux/Mac:  ./game_inventory

 The program starts with 6 pre-loaded sample items so you
 can test all features immediately.


 4. MENU OPTIONS
----------------------------------------------------------------

 +--------+---------------------------+--------------------------+
 | Option | Action                    | Description              |
 +--------+---------------------------+--------------------------+
 |   1    | Display Inventory         | Shows all items with     |
 |        |                           | ID, name, rarity, value, |
 |        |                           | quantity, and total value|
 +--------+---------------------------+--------------------------+
 |   2    | Add Item                  | Prompts for item name,   |
 |        |                           | rarity (0-4), value,     |
 |        |                           | and quantity             |
 +--------+---------------------------+--------------------------+
 |   3    | Remove Item               | Enter the item ID to     |
 |        |                           | remove it from inventory |
 +--------+---------------------------+--------------------------+
 |   4    | Search Item               | Search by item name      |
 |        |                           | (case-sensitive)         |
 +--------+---------------------------+--------------------------+
 |   5    | Update Quantity           | Enter item ID and new    |
 |        |                           | quantity. Auto-removes   |
 |        |                           | if quantity reaches 0    |
 +--------+---------------------------+--------------------------+
 |   6    | Sort Inventory            | Sort by:                 |
 |        |                           |   0 = Name (A-Z)         |
 |        |                           |   1 = Value (low-high)   |
 |        |                           |   2 = Rarity (common     |
 |        |                           |       to legendary)      |
 |        |                           |   3 = Quantity (low-high)|
 +--------+---------------------------+--------------------------+
 |   7    | Exit                      | Quit the program         |
 +--------+---------------------------+--------------------------+


 5. RARITY LEVELS
----------------------------------------------------------------

   0 = Common
   1 = Uncommon
   2 = Rare
   3 = Epic
   4 = Legendary


 6. EXAMPLE USAGE
----------------------------------------------------------------

 Starting the program:

   > game_inventory.exe

   Added: Iron Sword (ID 1)
   Added: Health Potion (ID 2)
   Added: Dragon Shield (ID 3)
   Added: Phoenix Feather (ID 4)
   Added: Steel Helmet (ID 5)
   Added: Mana Elixir (ID 6)

   === Game Inventory Management System ===
   Using Singly Linked List with Merge Sort

   --- Menu ---
   1. Display Inventory
   2. Add Item
   3. Remove Item (by ID)
   4. Search Item (by Name)
   5. Update Quantity
   6. Sort Inventory
   7. Exit
   Enter choice: 1

   ===========================================================
                      GAME INVENTORY
   ===========================================================
   ID   | Name                 | Rarity     | Value | Qty
   -----------------------------------------------------------
   6    | Mana Elixir          | Rare       | 200   | 3
   5    | Steel Helmet         | Uncommon   | 120   | 1
   4    | Phoenix Feather      | Legendary  | 1000  | 1
   3    | Dragon Shield        | Epic       | 500   | 1
   2    | Health Potion        | Common     | 25    | 5
   1    | Iron Sword           | Common     | 50    | 1
   -----------------------------------------------------------
   Total items: 6  |  Total inventory value: 2395
   ===========================================================


 7. DATA STRUCTURE DETAILS
----------------------------------------------------------------

 Time Complexity:
   - Add item (head):            O(1)
   - Remove item:                O(n)
   - Search item:                O(n)
   - Sort inventory (Merge Sort): O(n log n)
   - Display all items:          O(n)

 Space Complexity: O(n)
   Each node stores item data + one pointer to the next node.


================================================================
   END OF USER GUIDE
================================================================