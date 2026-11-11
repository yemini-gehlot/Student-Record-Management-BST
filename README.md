# Student Record Management System Using Binary Search Tree

## Project Overview
This project is a Student Record Management System developed using the C programming language and the Binary Search Tree (BST) data structure. It provides a graphical user interface using the Raylib library.

## Features
- Insert student records containing roll number, name, and marks.
- Search students using their roll numbers.
- Delete existing student records.
- Visualize the Binary Search Tree graphically.
- Display student roll numbers in ascending order using inorder traversal.

## Technologies Used
- C Programming Language
- Binary Search Tree (BST)
- Raylib Graphics Library
- GCC Compiler
- Visual Studio Code

## Time Complexity

| Operation | Average Case | Worst Case |
|---|---|---|
| Insertion | O(log n) | O(n) |
| Searching | O(log n) | O(n) |
| Deletion | O(log n) | O(n) |
| Inorder Traversal | O(n) | O(n) |

## How to Run
1. Install a compatible GCC compiler and Raylib library.
2. Open the project folder in VS Code.
3. Compile the C source file with GCC, linking Raylib and its Windows dependencies.
4. Run the generated executable.

## Future Enhancements
- File-based storage for student records.
- Updating existing student details.
- Self-balancing BST implementation.
- Improved graphical visualization.