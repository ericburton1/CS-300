//============================================================================
// Name        : ProjectTwo.cpp
// Author      : Eric Burton
// Version     : 1.0
// Description : Project Two - ABCU Course Program
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <limits>
#include <unordered_set>

using namespace std;

// ------------------------------
// Course struct
// ------------------------------

// Represents a single course record loaded from the CSV file
struct Course {
    string courseNumber;
    string courseTitle;
    vector<string> prerequisites;
};

// ------------------------------
// Helper functions
// ------------------------------

// Removes leading/trailing whitespace (helps handle " CSCI200 " formatting)
static string trim(const string& s) {
    size_t start = 0;
    while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }

    size_t end = s.size();
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }

    return s.substr(start, end - start);
}

// Normalizes course numbers to support case-insensitive lookup (e.g., "csci400")
static string toUpper(string s) {
        for (char& c : s) {
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        }
        return s;
    }

// Splits one CSV line into tokens (courseNumber, title, prereq1, prereq2, ...)
static vector<string> splitCSV(const string& line) {
    vector<string> tokens;
    string token;
    stringstream ss(line);
    while(getline(ss, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// ------------------------------
// BST for Courses (key = courseNumber)
// ------------------------------

// Binary Search Tree stores Course objects keyed by courseNumber.
// In-order traversal prints courses in alphanumeric order.
class BinarySearchTree {
private:
    struct Node {
        Course course;
        Node* left;
        Node* right;
        explicit Node(const Course& c) : course(c), left(nullptr), right(nullptr) {}
    };

    Node* root = nullptr;

    Node* insertNode(Node* node, const Course& course) {
        if (node == nullptr)
            return new Node(course);
        
        if (course.courseNumber < node->course.courseNumber) {
            node->left = insertNode(node->left, course);
        } else if (course.courseNumber > node->course.courseNumber) {
            node->right = insertNode(node->right, course);
        } else {
            // Duplicate: overwrite (policy)
            node->course = course;
        }
        return node;
    }

    const Course* searchNode(Node* node, const string& key) const {
        if (!node) return nullptr;
        if (key == node->course.courseNumber) 
            return &node->course;
        if (key < node->course.courseNumber)
            return searchNode(node->left, key);
        return searchNode(node->right, key);
    }

    void inOrder(Node* node) const {
        if (!node) return;
        inOrder(node->left);
        cout << node->course.courseNumber << ", " << node->course.courseTitle << endl;
        inOrder(node->right);
    }

    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    ~BinarySearchTree() {
        destroy(root);
    }

    void Clear() {
        destroy(root);
        root = nullptr;
    }

    bool IsEmpty() const {
        return root == nullptr;
    }

    void Insert(const Course& course) {
        root = insertNode(root, course);
    }

    const Course* Search(const string& courseNumber) const {
        return searchNode(root, courseNumber);
    }

    void PrintInOrder() const {
        inOrder(root);
    }
};

// Load courses into the BST using a two-pass approach
// Pass 1: read/validate lines and insert courses
// Pass 2: warn if a prerequisite course was referenced but never defined in the file
static bool loadCoursesToBST(const string& filename, BinarySearchTree& bst) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Unable to open file: " << filename << endl;
        return false;
    }

    bst.Clear();

    unordered_set<string> courseNumbers; // To track defined courses for prerequisite validation
    vector<pair<string, vector<string>>> prereqRefs; // Store courseNumber and its prereqs for second pass

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        lineNumber++;
        line = trim(line);
        if(line.empty()) continue; // Skip empty lines

        vector<string> tokens = splitCSV(line);
        if (tokens.size() < 2) {
            cout << "Format error on line " << lineNumber 
                 << ": each line must contain at least course number and title: " << endl;
            cout << "Line content: " << line << endl;
            continue; // Skip malformed line
        }

        Course c;
        c.courseNumber = toUpper(trim(tokens[0]));
        c.courseTitle = trim(tokens[1]);

        if (c.courseNumber.empty() || c.courseTitle.empty()) {
            cout << "Format error on line " << lineNumber
                 << ": course number and title cannot be blank" << endl;
            cout << "Line content: " << line << endl;
            continue; // Skip malformed line
        }

        // Warn on duplicates (BST insert will overwrite based on the policy above)
        if (courseNumbers.find(c.courseNumber) != courseNumbers.end()) {
            cout << "Warning on line " << lineNumber
                 << ": duplicate course number " << c.courseNumber
                 << " encountered. Existing course will be overwritten." << endl;
        }

        c.prerequisites.clear();
        for (size_t i = 2; i < tokens.size(); i++) {
            string prereq = toUpper(trim(tokens[i]));
            if (!prereq.empty()) c.prerequisites.push_back(prereq);
        }

        courseNumbers.insert(c.courseNumber);
        bst.Insert(c);
        prereqRefs.push_back({c.courseNumber, c.prerequisites});
    }

    // Second pass: check for undefined prerequisites
    for (const auto& entry : prereqRefs) {
        for (const string& prereq : entry.second) {
            if (courseNumbers.find(prereq) == courseNumbers.end()) {
                cout << "Prerequisite error: Course " << entry.first
                     << " lists prerequisite " << prereq
                     << ", but no matching course exists in the file." << endl;
            }
        }
    }

    return true;
}   

// Prints a single course and its prerequisites (including titles when available)
static void printCourse(const BinarySearchTree& bst, const string& input) {
    string key = toUpper(trim(input)); 
    const Course* course = bst.Search(key);

    if (!course) {
        cout << "Course not found: " << key << endl;
        return;
    }

    cout << course->courseNumber << ", " << course->courseTitle << endl;

    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None" << endl;
        return;
    }

    cout << "Prerequisites: ";
    for (size_t i = 0; i < course->prerequisites.size(); i++) { 
        const string& prereqNum = course->prerequisites[i];
        const Course* prereqCourse = bst.Search(prereqNum);
        
        if (prereqCourse) {
            cout << prereqCourse->courseNumber << " (" << prereqCourse->courseTitle << ")";
        } else {
            cout << prereqNum << " (missing in file)";
        }

        if (i + 1 < course->prerequisites.size()) cout << ", ";
    }
    cout << endl;
}

int main() {
    BinarySearchTree bst;
    bool dataLoaded = false;


    cout << "Welcome to the ABCU Course Program!" << endl;

    // Run menu loop
    while (true) {
        cout << "\nMenu:" << endl;
        cout << "1. Load Course Data" << endl;
        cout << "2. Print Course List" << endl;
        cout << "3. Print Course" << endl;
        cout << "9. Exit" << endl;
        cout << "Enter Choice (1-9): ";

        int choice;
        
        // Input guard prevents infinite loop if user types non-numeric input
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }

        switch (choice) {
            case 1: {
                // Load course data from file
                cout << "Please enter the name of the course file (e.g., ABCU_Advising_Program_Input.csv): ";
                string filename;
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear newline from input buffer
                getline(cin, filename);
                filename = trim(filename);
                dataLoaded = loadCoursesToBST(filename, bst);
                if (dataLoaded) {
                    cout << "Course data loaded successfully from " << filename << endl;
                } else {
                    cout << "Failed to load course data from " << filename << endl;
                }
                break;
            }

            case 2: 
                // Display all courses
                if (!dataLoaded || bst.IsEmpty()) {
                    cout << "No courses loaded. Please load data first." << endl;
                } else {
                    cout << "Here is a sample schedule:" << endl;
                    bst.PrintInOrder();
                }
                break;

            case 3:
                // Search for a course
                if (!dataLoaded || bst.IsEmpty()) {
                    cout << "No courses loaded. Please load data first." << endl;
                } else {
                    cout << "What course do you want to know about? ";
                    string courseNum;
                    cin >> courseNum;
                    printCourse(bst, courseNum);
                }
                break;

            case 9:
                cout << "Exiting the program. Goodbye!" << endl;
                return 0;

            default:
                cout << choice << " is not a valid option. Please try again." << endl;
        }
    }
}