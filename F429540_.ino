#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <Wire.h>

// ============================================================================
// CONSTANTS - Define all magic numbers here
// ============================================================================
const int MAX_ACCOUNTS = 35;
const int MAX_EMPLOYEE_ID_LENGTH = 7;
const int MIN_PAYGRADE = 1;
const int MAX_PAYGRADE = 9;
const int MIN_JOB_TITLE_LENGTH = 3;
const int MAX_JOB_TITLE_LENGTH = 17;
const float MIN_SALARY = 0.00;
const float MAX_SALARY = 99999.99;
const int BUTTON_HOLD_TIME = 1000;  // milliseconds
const int SERIAL_BAUD_RATE = 9600;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// Button pin definitions
const int BUTTON_UP = 2;
const int BUTTON_DOWN = 3;
const int BUTTON_SELECT = 4;

// Backlight color codes
const int BACKLIGHT_RED = 1;
const int BACKLIGHT_GREEN = 2;
const int BACKLIGHT_YELLOW = 3;
const int BACKLIGHT_WHITE = 7;
const int BACKLIGHT_PURPLE = 5;

// ============================================================================
// STRUCT DEFINITION
// ============================================================================
struct PayrollAccount {
    String employeeId;
    String employeePayGrade;
    String employeeJobTitle;
    float salary;
    String pensionStatus;
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
PayrollAccount payrollAccounts[MAX_ACCOUNTS];
int payrollAccountTotal = 0;
int currentEmployee = 0;
static unsigned long buttonSelectTime = 0;  // For button timing (static so it persists)

// ============================================================================
// VALIDATION HELPER FUNCTIONS
// ============================================================================

/**
 * Checks if a string contains only digits
 * @param str - String to validate
 * @return true if all characters are digits, false otherwise
 */
bool isNumber(const String& str) {
    for (int i = 0; i < str.length(); i++) {
        if (!isDigit(str[i])) {
            return false;
        }
    }
    return true;
}

/**
 * Validates and counts dashes in a message
 * Also checks for spaces (which are not allowed)
 * @param message - The serial message
 * @param expectedDashes - Number of dashes expected
 * @return true if validation passes, false otherwise
 */
bool validateMessageFormat(const String& message, int expectedDashes) {
    int dashCount = 0;
    
    // Count dashes
    for (int i = 0; i < message.length(); i++) {
        if (message[i] == '-') {
            dashCount++;
        }
    }
    
    // Check dash count
    if (dashCount != expectedDashes) {
        Serial.println("ERROR: Invalid format - incorrect number of dashes");
        return false;
    }
    
    // Check for spaces (not allowed)
    if (message.indexOf(" ") != -1) {
        Serial.println("ERROR: no spaces allowed");
        return false;
    }
    
    return true;
}

/**
 * Validates employee ID (must be 7 digits)
 * @param id - Employee ID to validate
 * @return true if valid, false otherwise
 */
bool isValidEmployeeId(const String& id) {
    if (id.length() != MAX_EMPLOYEE_ID_LENGTH) {
        Serial.println("ERROR: invalid employee id - must be 7 digits");
        return false;
    }
    return isNumber(id);
}

/**
 * Validates paygrade (must be 1-9)
 * @param grade - Paygrade to validate
 * @return true if valid, false otherwise
 */
bool isValidPaygrade(const String& grade) {
    if (grade.length() != 1 || !isNumber(grade)) {
        Serial.println("ERROR: invalid paygrade - must be single digit");
        return false;
    }
    int gradeValue = grade.toInt();
    if (gradeValue < MIN_PAYGRADE || gradeValue > MAX_PAYGRADE) {
        Serial.println("ERROR: invalid paygrade - must be between 1 and 9");
        return false;
    }
    return true;
}

/**
 * Validates job title (must be 3-17 characters)
 * @param title - Job title to validate
 * @return true if valid, false otherwise
 */
bool isValidJobTitle(const String& title) {
    if (title.length() < MIN_JOB_TITLE_LENGTH || title.length() > MAX_JOB_TITLE_LENGTH) {
        Serial.println("ERROR: invalid job title - must be 3-17 characters");
        return false;
    }
    return true;
}

/**
 * Validates salary (must be 0.00 - 99999.99)
 * @param salaryValue - Salary to validate
 * @return true if valid, false otherwise
 */
bool isValidSalary(float salaryValue) {
    if (salaryValue < MIN_SALARY || salaryValue > MAX_SALARY) {
        Serial.println("ERROR: Salary is not in valid range (0.00 - 99999.99)");
        return false;
    }
    return true;
}

/**
 * Validates pension status (must be "PEN" or "NPEN")
 * @param status - Pension status to validate
 * @return true if valid, false otherwise
 */
bool isValidPensionStatus(const String& status) {
    if (status != "PEN" && status != "NPEN") {
        Serial.println("ERROR: Invalid pension status - must be PEN or NPEN");
        return false;
    }
    return true;
}

// ============================================================================
// ACCOUNT MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * Finds an employee account by ID
 * @param employeeId - The ID to search for
 * @return index of account, or -1 if not found
 */
int findEmployeeAccount(const String& employeeId) {
    for (int i = 0; i < payrollAccountTotal; i++) {
        if (payrollAccounts[i].employeeId == employeeId) {
            return i;
        }
    }
    return -1;
}

/**
 * Sorts payroll accounts numerically by employee ID
 */
void sortPayrollAccounts() {
    for (int i = 0; i < payrollAccountTotal - 1; i++) {
        for (int j = 0; j < payrollAccountTotal - i - 1; j++) {
            if (payrollAccounts[j].employeeId > payrollAccounts[j + 1].employeeId) {
                // Swap accounts
                PayrollAccount temp = payrollAccounts[j];
                payrollAccounts[j] = payrollAccounts[j + 1];
                payrollAccounts[j + 1] = temp;
            }
        }
    }
}

/**
 * Ensures currentEmployee index is valid
 */
void validateCurrentEmployeeIndex() {
    if (payrollAccountTotal == 0) {
        currentEmployee = 0;
    } else if (currentEmployee >= payrollAccountTotal) {
        currentEmployee = payrollAccountTotal - 1;
    }
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

/**
 * ADD command: Adds a new employee
 * Format: ADD-{employeeId}-{paygrade}-{jobTitle}
 */
void handleAddEmployee(const String& message) {
    if (!validateMessageFormat(message, 3)) {
        return;
    }
    
    int dashOne = message.indexOf("-");
    int dashTwo = message.indexOf("-", dashOne + 1);
    int dashThree = message.indexOf("-", dashTwo + 1);
    
    String employeeId = message.substring(dashOne + 1, dashTwo);
    String payGrade = message.substring(dashTwo + 1, dashThree);
    String jobTitle = message.substring(dashThree + 1);
    
    // Validate employee ID
    if (!isValidEmployeeId(employeeId)) {
        return;
    }
    
    // Check if employee already exists
    if (findEmployeeAccount(employeeId) != -1) {
        Serial.println("ERROR: already an account");
        return;
    }
    
    // Validate paygrade
    if (!isValidPaygrade(payGrade)) {
        return;
    }
    
    // Validate job title
    if (!isValidJobTitle(jobTitle)) {
        return;
    }
    
    // Check if we have space for new account
    if (payrollAccountTotal >= MAX_ACCOUNTS) {
        Serial.println("ERROR: Maximum accounts reached");
        return;
    }
    
    // Add the new employee
    payrollAccounts[payrollAccountTotal].employeeId = employeeId;
    payrollAccounts[payrollAccountTotal].employeePayGrade = payGrade;
    payrollAccounts[payrollAccountTotal].employeeJobTitle = jobTitle;
    payrollAccounts[payrollAccountTotal].pensionStatus = "";  // Empty until salary is set
    payrollAccounts[payrollAccountTotal].salary = 0.00;
    
    payrollAccountTotal++;
    sortPayrollAccounts();
    Serial.println("DONE!");
}

/**
 * PST command: Updates pension status
 * Format: PST-{employeeId}-{pensionStatus}
 */
void handlePensionStatus(const String& message) {
    if (!validateMessageFormat(message, 2)) {
        return;
    }
    
    int dashOne = message.indexOf("-");
    int dashTwo = message.indexOf("-", dashOne + 1);
    
    String employeeId = message.substring(dashOne + 1, dashTwo);
    String pensionStatus = message.substring(dashTwo + 1);
    
    // Validate pension status
    if (!isValidPensionStatus(pensionStatus)) {
        return;
    }
    
    // Find employee
    int accountIndex = findEmployeeAccount(employeeId);
    if (accountIndex == -1) {
        Serial.println("ERROR: invalid employeeid");
        return;
    }
    
    // Validate salary is not 0.00
    if (payrollAccounts[accountIndex].salary == 0.00) {
        Serial.println("ERROR: salary is 0.00");
        return;
    }
    
    // Check if status is already the same
    if (payrollAccounts[accountIndex].pensionStatus == pensionStatus) {
        Serial.println("ERROR: Pension status is already the same");
        return;
    }
    
    // Update pension status
    payrollAccounts[accountIndex].pensionStatus = pensionStatus;
    Serial.println("DONE!");
}

/**
 * GRD command: Changes employee paygrade
 * Format: GRD-{employeeId}-{newPaygrade}
 * NOTE: New paygrade must be HIGHER than current paygrade
 */
void handleGradeChange(const String& message) {
    if (!validateMessageFormat(message, 2)) {
        return;
    }
    
    int dashOne = message.indexOf("-");
    int dashTwo = message.indexOf("-", dashOne + 1);  // FIX: Was using dashTwo instead of dashOne
    
    String employeeId = message.substring(dashOne + 1, dashTwo);
    String newPaygradeStr = message.substring(dashTwo + 1);
    
    // Validate paygrade
    if (!isValidPaygrade(newPaygradeStr)) {
        return;
    }
    
    int newPaygrade = newPaygradeStr.toInt();
    
    // Find employee
    int accountIndex = findEmployeeAccount(employeeId);
    if (accountIndex == -1) {
        Serial.println("ERROR: no account exists");
        return;
    }
    
    int currentPaygrade = payrollAccounts[accountIndex].employeePayGrade.toInt();
    
    // New paygrade must be higher than current
    if (currentPaygrade >= newPaygrade) {  // FIX: Logic was inverted
        Serial.println("ERROR: Same job grade or lower entered");
        return;
    }
    
    // Update paygrade
    payrollAccounts[accountIndex].employeePayGrade*
        }
