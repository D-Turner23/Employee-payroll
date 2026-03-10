#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <Wire.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

int buttonup = 2;
int buttondown = 3;
int buttonselect = 4; // define which buttons are assigned on the arduino

bool isNumber(String str) {
  // For converting salary to a float
  for (int i = 0; i < str.length(); i++) {
  
    if (!isDigit(str[i])) {
      return false; // If any character is not a digit then the salary would be invalid
    }
  }
  return true; // All characters are digits the salary would be valid
}

// Defining the payroll account
struct Payroll_account {
  String employeeid; // employeeidnumber
  String employeepaygrade; // employee pay grade
  String employeejobtitle; // employee job title
  float salary; // employee salary
  String pensionstatus; // employee pension status
};
const int Max_accounts = 35; // max number of students

Payroll_account Payroll_accounts[Max_accounts]; // Hold account data
int Payroll_accounttotal= 0; // ammount of accounts 
int currentemployee = 0; // account on the lcd at the time

void sortPayroll_accounts() { // sorting payroll accounts numerically
  for (int i = 0; i < currentemployee - 1; i++) {
    for (int j = 0; j < currentemployee - i - 1; j++) {
      if (Payroll_accounts[j].employeeid > Payroll_accounts[j + 1].employeeid) {
        // Swap students[j] and students[j + 1]
        Payroll_account temp = Payroll_accounts[j];
        Payroll_accounts[j] = Payroll_accounts[j + 1];
        Payroll_accounts[j + 1] = temp;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);  // Start 
  lcd.begin(16, 2);
  lcd.setBacklight(3); // yellow backlight
   





  // Send R every two seconds
  while (true) {
    Serial.print("R");  //  Send "R" 
    delay(2000);         // Wait for 2 seconds before sending again
    
    // Check if there's any data from the Serial buffer
    if (Serial.available() > 0) {
      String begin = Serial.readStringUntil('\n'); // Read until newline character
      
      // Check if the received string is "BEGIN"
      if (begin == "BEGIN") {
        // Synchronization complete
        Serial.println("BASIC");  // Send "BASIC" 
        lcd.setBacklight(7);
             // Set backlight to white
        
        // Exit the loop after synchronization
        break;
      }
      
      else {
        Serial.println("ERROR");  // Print error message if anything beside BEGIN is sent
        // Keep sending 'R' until "BEGIN" is received correctly
      }

    }
  }
}

void loop() {
   
    sortPayroll_accounts();
    uint8_t buttons = lcd.readButtons(); // function for button presses
    unsigned long buttonselecttime = 0; // set time

    if (buttons & buttonselect) {
    if (millis() - buttonselecttime > 1000) { // if select button has been pressed down for longer than 1 second
        lcd.clear();
        lcd.setBacklight(5); // to show student id with purple backlight
        lcd.print("F429540"); // student id
    }
} else {
    // Clear the screen and default backlight (7 is white)
    lcd.clear();
    lcd.setBacklight(7);  // Default white or neutral backlight
    
    if (Payroll_accounttotal > 0 && currentemployee < Payroll_accounttotal) {
        // Set the backlight based on the pension status before displaying employee info
        if (Payroll_accounts[currentemployee].pensionstatus == "PEN") {
            lcd.setCursor(4,0);
            lcd.print(Payroll_accounts[currentemployee].pensionstatus);
            lcd.setBacklight(2);  // Green backlight for PEN
        } else if (Payroll_accounts[currentemployee].pensionstatus == "NPEN") {
          lcd.setCursor(3, 0);
        lcd.print(Payroll_accounts[currentemployee].pensionstatus); // Show the pension status
            lcd.setBacklight(1);  // Red backlight for NPEN
        }
        
        // Display employee information
        lcd.setCursor(0, 0);
        lcd.print("^");  // to show scrolling
        lcd.setCursor(1, 0);
        lcd.print(Payroll_accounts[currentemployee].employeepaygrade); // Show the paygrade
        lcd.setCursor(8, 0);
        lcd.print(Payroll_accounts[currentemployee].salary); // Show the salary
        lcd.setCursor(0, 1);
        lcd.print("v");  // show you can scroll down
        lcd.setCursor(1, 1);
        lcd.print(Payroll_accounts[currentemployee].employeeid); // Show the employee id
        lcd.setCursor(9, 1);
        lcd.print(Payroll_accounts[currentemployee].employeejobtitle); // Show the employee job title
    }
 }
    if (buttons & buttonup){
      if (currentemployee < Payroll_accounttotal - 1) {
        currentemployee++;
    }
    }
    if (buttons & buttondown){
      if (currentemployee > 0) {
        currentemployee--;
    }
    }

    //  Read the message from Serial input
    String message = Serial.readStringUntil('\n');

    if (message.length() == 0) {
        return;  // do nothing if nothing is typed
    }

    // Add new employee
    if (message.startsWith("ADD")) {
        int dashone = message.indexOf("-");
        int dashtwo = message.indexOf("-", dashone + 1);
        int dashthree = message.indexOf("-", dashtwo + 1); // define the dashes

        int dashes = 0;
        for (int i = 0; i < message.length(); i++) {
          if (message[i] == '-') { // define way to count dashes
            dashes++;
         }
       }
    // Check if there are exactly 3 dashes 
        if (dashes != 3) {
        Serial.println("ERROR: Invalid format"); // show error
        return;
    }
    if (message.indexOf(" ") != -1){ // check for spaces
      Serial.println("ERROR: no spaces"); // show error
      return;
    }

        if (dashone != -1 && dashtwo != -1 && dashthree != -1) {
            String employeeid = message.substring(dashone + 1, dashtwo); 
            String employeepaygrade = message.substring(dashtwo + 1, dashthree);
            String employeejobtitle = message.substring(dashthree + 1); // define whre each string must go

            
            // Validate employee ID (7 digits)
            if (employeeid.length() == 7 && isNumber(employeeid)) {
                // Check for existing employee 
                bool account = false;
                for (int i = 0; i < Payroll_accounttotal; i++) {
                    if (Payroll_accounts[i].employeeid == employeeid) {
                        account = true; // an account exists with the employee id given
                        break;
                    }
                }
                if (account) {
                    Serial.println("ERROR: already an account"); // show error for adding account with same number
                    return;
                }

                // Validate paygrade (1-9)
                if (employeepaygrade.length() == 1 && isNumber(employeepaygrade) && employeepaygrade.toInt() >= 1 && employeepaygrade.toInt() <= 9) {
                    // Validate job title length (3-17 characters)
                    if (employeejobtitle.length() >= 3 && employeejobtitle.length() <= 17) {
                        // Add new employee
                        Payroll_accounts[Payroll_accounttotal].employeeid = employeeid; // add employee id
                        Payroll_accounts[Payroll_accounttotal].employeepaygrade = employeepaygrade; // add paygrade
                        Payroll_accounts[Payroll_accounttotal].employeejobtitle = employeejobtitle; // add job title
                        Payroll_accounts[Payroll_accounttotal].pensionstatus = ""; // leave pensions status until salary is given
                        Payroll_accounts[Payroll_accounttotal].salary = 0.00;  // Default salary

                        Payroll_accounttotal++;  // increase the total number of accounts by one
                        Serial.println("DONE!"); // show it was successful
                        return;
                    } else {
                        Serial.println("ERROR: invalid job title"); // error for invalid job title
                    }
                } else {
                    Serial.println("ERROR: invalid paygrade"); // error for invalid paygrade
                }
            } else {
                Serial.println("ERROR: invalid employee id"); // error for invalid number
            }
        } 
    }



    // Update pension status
    if (message.startsWith("PST")) {
        int dashone = message.indexOf("-");
        int dashtwo = message.indexOf("-", dashone + 1); // define dashes

        int dashes = 0;
        for (int i = 0; i < message.length(); i++) {
          if (message[i] == '-') {// counting for dashes
            dashes++; // add one to count
        }
    }
    // Check if there are exactly 2 dashes 
    if (dashes != 2) {
        Serial.println("ERROR: Invalid format");
        return;
    }
    if (message.indexOf(" ") != -1){ // check for spaces
      Serial.println("ERROR: no spaces"); //error for having spaces
      return;
    }

        if (dashone != -1 && dashtwo != -1) {
            String employeeid = message.substring(dashone + 1, dashtwo); // show position of id
            String pensionstatus = message.substring(dashtwo + 1); // show position of pensionstatus

            // Validate pension status (must be "PEN" or "NPEN")
            if (pensionstatus != "PEN" && pensionstatus != "NPEN") { // status must be one of two values
                Serial.println("ERROR: Invalid pension status"); // error for anything else
                return;
            }

            bool employee = false;
            for (int i = 0; i < Payroll_accounttotal; i++) { // check through struct
                if (Payroll_accounts[i].employeeid == employeeid) { //check for existing employee
                    employee = true; // an account exists with the employee id given

                    if (Payroll_accounts[i].salary != 0.00) { // salary must be above 0.00 for pension
                        if (Payroll_accounts[i].pensionstatus == pensionstatus) {
                            Serial.println("ERROR: Pension status is already the same"); // error for 0.00 salary
                            return;
                        }
                        Payroll_accounts[i].pensionstatus = pensionstatus; // set new pension status
                        Serial.println("DONE!"); // show successgful
                        return;
                    } else {
                        Serial.println("ERROR: salary is 0.00"); // invalid salary
                        return;
                    }
                }
            }

            if (!employee) {
                Serial.println("ERROR: invalid employeeid"); // employee id is not registered
            }
        } 
    }

    
    // Change employee paygrade
    if (message.startsWith("GRD")) { // check for protocol
      int dashone = message.indexOf("-"); // define dashes
      int dashtwo = message.indexOf("-", dashtwo + 1);

      int dashes = 0; // create loop to count dashes
      for (int i = 0; i < message.length(); i++) {
        if (message[i] == '-') { // check for dashes
            dashes++; // add one to total
         }
       }
    // Check if there are  2 dashes in the message 
        if (dashes != 2) {
        Serial.println("ERROR: Invalid format"); // error for not having two
        return;
    }
    if (message.indexOf(" ") != -1){ // check for spaces
      Serial.println("ERROR: no spaces"); // error if there are spaces
      return; 
    }

    if (dashone != -1 && dashtwo != -1) {
        String employeeid = message.substring(dashone + 1, dashtwo); // check if id is in right place
        String employeepaygrade = message.substring(dashtwo + 1); // check if paygrade is in right place

        // Ensure the paygrade  within the range 1-9
        int newPaygrade = employeepaygrade.toInt(); // convert to integer
        if (newPaygrade > 1 && newPaygrade < 9) { 
            Serial.println("ERROR: invalid paygrade");// if not 1-9
            return;
        }

        for (int i = 0; i < Payroll_accounttotal; i++) {
            if (Payroll_accounts[i].employeeid == employeeid) { // check for employeeid
                int currentPaygrade = Payroll_accounts[i].employeepaygrade.toInt(); // assign new paygrade

                if (currentPaygrade >= newPaygrade) { // check new paygrade is higher than old paygrade
                    Serial.println("ERROR: Same job grade or lower entered"); // error if it isn't
                    return; 
                }

                Payroll_accounts[i].employeepaygrade = employeepaygrade;
                Serial.println("DONE!"); // assign new paygrade and print done
                return;
            }
        }

        Serial.println("ERROR: no account exists"); // if employeeid isn't found
    }
}

    
    // Set employee salary
    if (message.startsWith("SAL")) {
    // Locate dashes in the message
    int dashone = message.indexOf("-"); // define dashes
    int dashtwo = message.indexOf("-", dashone + 1);

    int dashes = 0;
    for (int i = 0; i < message.length(); i++) {
        if (message[i] == '-') { // create way of checking for number of dashes
            dashes++;
        }
    }

    // Check if there are  2 dashes
    if (dashes != 2) {
        Serial.println("ERROR: Invalid format"); //error for not having two
        return;
    }
    if (message.indexOf(" ") != -1) { //check for spaces
        Serial.println("ERROR: no spaces allowed"); // error for having spaces
        return; 
    }

    // check for valid dash positions
    if (dashone != -1 && dashtwo != -1) {
        String employeeid = message.substring(dashone + 1, dashtwo); // check id is in right place
        String salary = message.substring(dashtwo + 1); // check employee salary is in the right place

        

        // Convert  salary to float
        float Salary = salary.toFloat();

        bool account = false;
        for (int i = 0; i < Payroll_accounttotal; i++) {
            //  Check the employee ID 
            

            // Ensure id exists
            if (Payroll_accounts[i].employeeid == employeeid) {
                account = true;

                // Check if the salary is within the valid range
                if (Salary >= 0.00 && Salary <= 99999.99) {
                    Payroll_accounts[i].salary = Salary; //assign new salary
                    Serial.println("DONE!"); // show successful
                    return;
                } else {
                    Serial.println("ERROR: Salary is not in valid range"); //error for invalid salary
                    return;
                }
            }
        }

        if (!account) {
            Serial.println("ERROR: no account found"); // error for account not existing
        }
    }
}

    

    // Set employee job title
    if (message.startsWith("CJT")) {
        int dashone = message.indexOf("-");
        int dashtwo = message.indexOf("-", dashone + 1); // define dashes

        int dashes = 0;
        for (int i = 0; i < message.length(); i++) {
          if (message[i] == '-') { // create way of checking for dashes
            dashes++; // add one to total 
         }
       }
    // Check if there are 2 dashes
        if (dashes != 2) {
        Serial.println("ERROR: Invalid format"); //error for not having two
        return;
    }
    if (message.indexOf(" ") != -1){ //check for spaces
      Serial.println("ERROR: no spaces"); // error for having spaces
      return; 
    }

        if (dashone != -1 && dashtwo != -1) {
            String employeeid = message.substring(dashone + 1, dashtwo); // check id is in right place
            String employeejobtitle = message.substring(dashtwo + 1); // check job title is in right place

            for (int i = 0; i < Payroll_accounttotal; i++) { // go through each account in struct
                if (Payroll_accounts[i].employeeid == employeeid) { //check account exists
                    if (employeejobtitle.length() >= 3 && employeejobtitle.length() <= 17) {// check for valid job title
                        Payroll_accounts[i].employeejobtitle = employeejobtitle; // assign new job title
                        Serial.println("DONE!");
                        return;
                    } else {
                        Serial.println("ERROR: invalid job title"); // error for invalid job title
                        return;
                    }
                }
            }

            Serial.println("ERROR: account not found"); // error for account not existing
        } 
    }

    //  Delete employee account
    if (message.startsWith("DEL")) {
        int dashone = message.indexOf("-");

        int dashes = 0;
        for (int i = 0; i < message.length(); i++) {
          if (message[i] == '-') {
            dashes++;
         }
       }
    // Check if there is one dashes 
        if (dashes != 1) {
        Serial.println("ERROR: Invalid format");
        return;
    }
    if (message.indexOf(" ") != -1){ // check for spaces by splitting string
      Serial.println("ERROR: no spaces"); // error for having spaces
      return;
    }

        if (dashone != -1) {
            String employeeid = message.substring(dashone + 1); // check employee id is in right place

            for (int i = 0; i < Payroll_accounttotal; i++) { // search through struct
                if (Payroll_accounts[i].employeeid == employeeid) { // check account exists
                    // Shift the array to delete the account
                    for (int j = i; j < Payroll_accounttotal - 1; j++) { // this for loop is ai generated by chat gpt
                        Payroll_accounts[j] = Payroll_accounts[j + 1]; // shift the struct so account gets deleted
                    }
                    Payroll_accounttotal--; // take one from account total
                    Serial.println("DONE!");
                    return;
                }
            }

            Serial.println("ERROR: account not found"); // error for invalid id
        }
    }
}

