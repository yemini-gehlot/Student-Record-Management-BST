#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#define SCREEN_W 1100
#define SCREEN_H 750
#define MAX_NAME 64
#define FIELD_SIZE 128

typedef struct Student {
    int roll;
    char name[MAX_NAME];
    float marks;
    float x, y;
    struct Student *left, *right;
} Student;

typedef enum { FIELD_NONE = -1, FIELD_ROLL = 0, FIELD_NAME = 1, FIELD_MARKS = 2 } Field;

static Student *root = NULL;
static int studentCount = 0;
static char rollInput[FIELD_SIZE] = "";
static char nameInput[FIELD_SIZE] = "";
static char marksInput[FIELD_SIZE] = "";
static char status[256] = "Enter student details, then click Insert Student.";
static char traversal[2048] = "";
static Field activeField = FIELD_ROLL;

// Prefer a real bold TrueType font for crisp, readable labels.
static Font uiFont;
static bool customFontLoaded = false;

static void drawBoldText(const char *text, int x, int y, int size, Color color) {
    if (customFontLoaded) {
        DrawTextEx(uiFont, text, (Vector2){(float)x, (float)y}, (float)size, 0.5f, color);
    } else {
        // Portable fallback if the Windows font file is unavailable.
        DrawText(text, x, y, size, color);
        DrawText(text, x + 1, y, size, color);
    }
}

static int boldTextWidth(const char *text, int size) {
    if (customFontLoaded) return (int)MeasureTextEx(uiFont, text, (float)size, 0.5f).x;
    return MeasureText(text, size) + 1;
}

static Student *createStudent(int roll, const char *name, float marks) {
    Student *node = (Student *)malloc(sizeof(Student));
    if (!node) return NULL;
    node->roll = roll;
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->marks = marks;
    node->x = node->y = 0;
    node->left = node->right = NULL;
    return node;
}

static Student *insertStudent(Student *node, int roll, const char *name, float marks, bool *added) {
    if (!node) {
        Student *created = createStudent(roll, name, marks);
        if (created) { *added = true; studentCount++; }
        return created;
    }
    if (roll < node->roll) node->left = insertStudent(node->left, roll, name, marks, added);
    else if (roll > node->roll) node->right = insertStudent(node->right, roll, name, marks, added);
    return node;
}

static Student *searchStudent(Student *node, int roll) {
    if (!node || node->roll == roll) return node;
    return roll < node->roll ? searchStudent(node->left, roll) : searchStudent(node->right, roll);
}

static Student *minimumNode(Student *node) {
    while (node && node->left) node = node->left;
    return node;
}

static Student *deleteStudent(Student *node, int roll, bool *deleted) {
    if (!node) return NULL;
    if (roll < node->roll) node->left = deleteStudent(node->left, roll, deleted);
    else if (roll > node->roll) node->right = deleteStudent(node->right, roll, deleted);
    else {
        *deleted = true;
        if (!node->left || !node->right) {
            Student *child = node->left ? node->left : node->right;
            free(node);
            studentCount--;
            return child;
        }
        Student *successor = minimumNode(node->right);
        node->roll = successor->roll;
        snprintf(node->name, sizeof(node->name), "%s", successor->name);
        node->marks = successor->marks;
        bool removedSuccessor = false;
        node->right = deleteStudent(node->right, successor->roll, &removedSuccessor);
    }
    return node;
}

static void freeTree(Student *node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

static void buildTraversal(Student *node) {
    if (!node) return;
    buildTraversal(node->left);
    char piece[32];
    snprintf(piece, sizeof(piece), "%d ", node->roll);
    if (strlen(traversal) + strlen(piece) < sizeof(traversal)) strcat(traversal, piece);
    buildTraversal(node->right);
}

static void layoutTree(Student *node, int depth, int *index) {
    if (!node) return;
    layoutTree(node->left, depth + 1, index);
    (*index)++;
    node->x = 45.0f + (*index) * (SCREEN_W - 90.0f) / (studentCount + 1);
    node->y = 270.0f + depth * 85.0f;
    layoutTree(node->right, depth + 1, index);
}

static void drawEdges(Student *node) {
    if (!node) return;
    if (node->left) {
        DrawLineEx((Vector2){node->x, node->y + 26},
                   (Vector2){node->left->x, node->left->y - 26}, 2, GRAY);
        drawEdges(node->left);
    }
    if (node->right) {
        DrawLineEx((Vector2){node->x, node->y + 26},
                   (Vector2){node->right->x, node->right->y - 26}, 2, GRAY);
        drawEdges(node->right);
    }
}

static void drawNodes(Student *node) {
    if (!node) return;
    DrawCircle((int)node->x, (int)node->y, 27, (Color){218, 235, 255, 255});
    DrawCircleLines((int)node->x, (int)node->y, 27, BLUE);
    char rollText[32], marksText[32];
    snprintf(rollText, sizeof(rollText), "%d", node->roll);
    snprintf(marksText, sizeof(marksText), "%.1f", node->marks);
    drawBoldText(rollText, (int)node->x - boldTextWidth(rollText, 17)/2, (int)node->y - 9, 17, DARKBLUE);
    drawBoldText(node->name, (int)node->x - boldTextWidth(node->name, 12)/2, (int)node->y + 31, 12, BLACK);
    drawBoldText(marksText, (int)node->x - boldTextWidth(marksText, 12)/2, (int)node->y + 45, 12, DARKGRAY);
    drawNodes(node->left);
    drawNodes(node->right);
}

static bool button(Rectangle rect, const char *label) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, hover ? SKYBLUE : LIGHTGRAY);
    DrawRectangleLinesEx(rect, 1, DARKGRAY);
    drawBoldText(label, (int)rect.x + 10, (int)rect.y + 11, 16, DARKGRAY);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void clearFields(void) {
    rollInput[0] = nameInput[0] = marksInput[0] = '\0';
    activeField = FIELD_ROLL;
}

static void drawInput(Rectangle rect, const char *label, const char *value, bool focused) {
    drawBoldText(label, (int)rect.x, (int)rect.y - 21, 16, DARKGRAY);
    DrawRectangleRec(rect, focused ? (Color){235, 245, 255, 255} : (Color){245, 247, 250, 255});
    DrawRectangleLinesEx(rect, focused ? 2 : 1, focused ? BLUE : GRAY);
    drawBoldText(value, (int)rect.x + 9, (int)rect.y + 10, 18, BLACK);
    if (focused && (int)(GetTime() * 2) % 2 == 0) {
        int caretX = (int)rect.x + 9 + boldTextWidth(value, 18);
        if (caretX < rect.x + rect.width - 6) DrawLine(caretX, (int)rect.y + 8, caretX, (int)rect.y + 31, DARKBLUE);
    }
}

static bool parseRoll(int *roll) {
    if (!rollInput[0]) return false;
    errno = 0;
    char *end;
    long value = strtol(rollInput, &end, 10);
    if (errno == ERANGE || *end != '\0' || value < 0 || value > INT_MAX) return false;
    *roll = (int)value;
    return true;
}

static void doInsert(void) {
    int roll;
    if (!parseRoll(&roll)) {
        snprintf(status, sizeof(status), "Enter a valid non-negative roll number.");
        activeField = FIELD_ROLL;
        return;
    }
    if (!nameInput[0] || strlen(nameInput) >= MAX_NAME) {
        snprintf(status, sizeof(status), "Enter a name (1 to %d characters).", MAX_NAME - 1);
        activeField = FIELD_NAME;
        return;
    }
    if (!marksInput[0]) {
        snprintf(status, sizeof(status), "Enter marks between 0 and 100.");
        activeField = FIELD_MARKS;
        return;
    }
    errno = 0;
    char *end;
    float marks = strtof(marksInput, &end);
    if (errno == ERANGE || *end != '\0' || !(marks >= 0.0f && marks <= 100.0f)) {
        snprintf(status, sizeof(status), "Marks must be a number from 0 to 100.");
        activeField = FIELD_MARKS;
        return;
    }
    if (searchStudent(root, roll)) {
        snprintf(status, sizeof(status), "Roll number %d already exists.", roll);
        return;
    }
    bool added = false;
    root = insertStudent(root, roll, nameInput, marks, &added);
    if (added) {
        snprintf(status, sizeof(status), "Added %s (Roll %d, Marks %.1f).", nameInput, roll, marks);
        clearFields();
    } else {
        snprintf(status, sizeof(status), "Memory allocation failed.");
    }
}

static void doSearch(void) {
    int roll;
    if (!parseRoll(&roll)) {
        snprintf(status, sizeof(status), "Enter a valid roll number to search.");
        activeField = FIELD_ROLL;
        return;
    }
    Student *found = searchStudent(root, roll);
    if (found) snprintf(status, sizeof(status), "Found: %s | Roll: %d | Marks: %.1f", found->name, found->roll, found->marks);
    else snprintf(status, sizeof(status), "Roll number %d not found.", roll);
}

static void doDelete(void) {
    int roll;
    if (!parseRoll(&roll)) {
        snprintf(status, sizeof(status), "Enter a valid roll number to delete.");
        activeField = FIELD_ROLL;
        return;
    }
    bool deleted = false;
    root = deleteStudent(root, roll, &deleted);
    if (deleted) snprintf(status, sizeof(status), "Deleted roll number %d.", roll);
    else snprintf(status, sizeof(status), "Roll number %d not found.", roll);
}

static void handleKeyboard(void) {
    if (IsKeyPressed(KEY_TAB)) activeField = (Field)((activeField + 1) % 3);
    if (IsKeyPressed(KEY_ENTER)) {
        if (activeField == FIELD_ROLL) activeField = FIELD_NAME;
        else if (activeField == FIELD_NAME) activeField = FIELD_MARKS;
        else doInsert();
        return;
    }
    char *target = activeField == FIELD_ROLL ? rollInput :
                   activeField == FIELD_NAME ? nameInput : marksInput;
    int ch = GetCharPressed();
    while (ch > 0) {
        size_t len = strlen(target);
        bool allowed = false;
        if (activeField == FIELD_ROLL) allowed = (ch >= '0' && ch <= '9');
        else if (activeField == FIELD_MARKS) allowed = (ch >= '0' && ch <= '9') || (ch == '.' && !strchr(target, '.'));
        else allowed = ch >= 32 && ch <= 126;
        if (allowed && len < FIELD_SIZE - 1 && (activeField != FIELD_NAME || len < MAX_NAME - 1)) {
            target[len] = (char)ch;
            target[len + 1] = '\0';
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t len = strlen(target);
        if (len > 0) target[len - 1] = '\0';
    }
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Student Record Management - BST");
    SetTargetFPS(60);
    // Standard Windows bold font; no font file needs to be copied into the project.
    if (FileExists("C:/Windows/Fonts/arialbd.ttf")) {
        uiFont = LoadFontEx("C:/Windows/Fonts/arialbd.ttf", 48, NULL, 0);
        if (uiFont.texture.id != 0) {
            customFontLoaded = true;
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
        }
    }

    Rectangle rollRect = {20, 86, 175, 40};
    Rectangle nameRect = {210, 86, 320, 40};
    Rectangle marksRect = {545, 86, 170, 40};
    Rectangle insertRect = {20, 145, 160, 40};
    Rectangle searchRect = {190, 145, 155, 40};
    Rectangle deleteRect = {355, 145, 155, 40};
    Rectangle clearRect = {520, 145, 150, 40};

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, rollRect)) activeField = FIELD_ROLL;
            else if (CheckCollisionPointRec(mouse, nameRect)) activeField = FIELD_NAME;
            else if (CheckCollisionPointRec(mouse, marksRect)) activeField = FIELD_MARKS;
        }
        handleKeyboard();
        int index = 0;
        layoutTree(root, 0, &index);
        traversal[0] = '\0';
        buildTraversal(root);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawBoldText("STUDENT RECORD MANAGEMENT SYSTEM", 20, 16, 25, DARKBLUE);
        drawBoldText("Click a field to type. Tab changes fields; Enter advances and inserts after Marks.", 20, 49, 15, DARKGRAY);
        drawInput(rollRect, "Roll Number", rollInput, activeField == FIELD_ROLL);
        drawInput(nameRect, "Student Name", nameInput, activeField == FIELD_NAME);
        drawInput(marksRect, "Marks (0-100)", marksInput, activeField == FIELD_MARKS);
        if (button(insertRect, "Insert Student")) doInsert();
        if (button(searchRect, "Search Roll")) doSearch();
        if (button(deleteRect, "Delete Roll")) doDelete();
        if (button(clearRect, "Clear Input")) {
            clearFields();
            snprintf(status, sizeof(status), "Input cleared.");
        }
        drawBoldText(status, 20, 200, 16, MAROON);
        drawBoldText("BINARY SEARCH TREE VISUALIZATION", 20, 235, 18, DARKBLUE);
        if (!root) drawBoldText("No student records yet. Enter details and click Insert Student.", 200, 390, 19, GRAY);
        else { drawEdges(root); drawNodes(root); }
        DrawRectangle(0, 689, SCREEN_W, 61, (Color){245, 247, 250, 255});
        drawBoldText("Inorder (ascending roll numbers):", 20, 685, 22, DARKBLUE);
        drawBoldText(traversal, 20, 718, 24, DARKGRAY);
        EndDrawing();
    }
    freeTree(root);
    if (customFontLoaded) UnloadFont(uiFont);
    CloseWindow();
    return 0;
}
