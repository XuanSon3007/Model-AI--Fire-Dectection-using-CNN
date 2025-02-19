#include <iostream>
using namespace std;

// Hàm xóa các ký tự trùng lặp từ chuỗi
string removeDuplicates(string str) {
    string result = "";
    for (char c : str) {
        if (result.find(c) == string::npos) {
            result += c;
        }
    }
    return result;
}

// Hàm tạo ma trận khóa 5x5
void createKeyMatrix(string key, char matrix[5][5]) {
    // Chuyển khóa thành chữ hoa và xóa ký tự trùng lặp
    for (char& c : key) {
        c = toupper(c);
    }
    key = removeDuplicates(key);

    // Tạo chuỗi bảng chữ cái không bao gồm 'J' (I và J được xem như nhau)
    string alphabet = "";
    for (char c = 'A'; c <= 'Z'; c++) {
        if (c != 'J') {
            alphabet += c;
        }
    }

    // Xóa các ký tự khóa khỏi bảng chữ cái
    for (char c : key) {
        size_t pos = alphabet.find(c);
        if (pos != string::npos) {
            alphabet.erase(pos, 1);
        }
    }

    // Điền ma trận với khóa và các chữ cái còn lại
    string fullKey = key + alphabet;
    int k = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            matrix[i][j] = fullKey[k++];
        }
    }
}

// Hàm định dạng văn bản gốc
string formatPlaintext(string text) {
    string result = "";
    for (char& c : text) {
        c = toupper(c);
        if (c == 'J') c = 'I';
        if (isalpha(c)) result += c;
    }
    
    // Xử lý các chữ cái trùng lặp bằng cách chèn 'X'
    string formatted = "";
    for (size_t i = 0; i < result.length(); i++) {
        formatted += result[i];
        if (i < result.length() - 1) {
            if (result[i] == result[i + 1]) {
                formatted += 'X';
            }
        }
    }
    
    // Thêm 'X' nếu độ dài là số lẻ
    if (formatted.length() % 2 != 0) {
        formatted += 'X';
    }
    
    return formatted;
}

// Hàm tìm vị trí của ký tự trong ma trận
void findPosition(char matrix[5][5], char c, int& row, int& col) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (matrix[i][j] == c) {
                row = i;
                col = j;
                return;
            }
        }
    }
}

// Hàm mã hóa một cặp ký tự
string encryptPair(char matrix[5][5], char a, char b) {
    int row1, col1, row2, col2;
    findPosition(matrix, a, row1, col1);
    findPosition(matrix, b, row2, col2);
    
    string result = "";
    
    if (row1 == row2) { // Cùng hàng
        result += matrix[row1][(col1 + 1) % 5];
        result += matrix[row2][(col2 + 1) % 5];
    }
    else if (col1 == col2) { // Cùng cột
        result += matrix[(row1 + 1) % 5][col1];
        result += matrix[(row2 + 1) % 5][col2];
    }
    else { // Trường hợp hình chữ nhật
        result += matrix[row1][col2];
        result += matrix[row2][col1];
    }
    
    return result;
}

// Hàm mã hóa toàn bộ thông điệp
string encrypt(char matrix[5][5], string text) {
    string ciphertext = "";
    for (size_t i = 0; i < text.length(); i += 2) {
        ciphertext += encryptPair(matrix, text[i], text[i + 1]);
    }
    return ciphertext;
}

int main() {
    string plaintext, key;
    char matrix[5][5];
    
    // Nhận đầu vào
    cout << "Enter plaintext: ";
    getline(cin, plaintext);
    cout << "Enter key: ";
    getline(cin, key);
    
    // Tạo ma trận khóa
    createKeyMatrix(key, matrix);
    
    // Hiển thị ma trận khóa
    cout << "\n5x5 Key Matrix:\n";
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
    
    // Định dạng văn bản gốc và mã hóa
    string formattedText = formatPlaintext(plaintext);
    string ciphertext = encrypt(matrix, formattedText);
    
    // Hiển thị kết quả
    cout << "\nFormatted plaintext: " << formattedText << endl;
    cout << "Ciphertext: " << ciphertext << endl;

    return 0;
}