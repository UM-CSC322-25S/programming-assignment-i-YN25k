#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_LINE_LENGTH 256

// Define boat types
typedef enum {
    SLIP,
    LAND,
    TRAILOR,
    STORAGE
} BoatType;

// Union for extra info
typedef union {
    int slip_number;           // For SLIP (1-85)
    char bay_letter;           // For LAND (A-Z)
    char trailer_license[20];  // For TRAILOR (license tag)
    int storage_number;        // For STORAGE (1-50)
} ExtraInfo;

// Boat struct definition
typedef struct {
    char name[128];  // Up to 127 characters plus null terminator
    float length;    // Length in feet (up to 100')
    BoatType type;   // Location type of the boat
    ExtraInfo info;  // Extra info based on the boat type
    float amountOwed;
} Boat;

// Global array of pointers to Boat
Boat *boats[MAX_BOATS];
int boatCount = 0;

// Helper: trim whitespace from the beginning and end of a string.
char* trimWhitespace(char* str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0)  // All spaces?
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = '\0';
    return str;
}

// Compare two boats by name (case-insensitive)
int compareBoats(const void *a, const void *b) {
    Boat *boatA = *(Boat **)a;
    Boat *boatB = *(Boat **)b;
    return strcasecmp(boatA->name, boatB->name);
}

// Convert a type string to a BoatType enum.
BoatType getBoatType(const char *typeStr) {
    if (strcasecmp(typeStr, "slip") == 0)
        return SLIP;
    else if (strcasecmp(typeStr, "land") == 0)
        return LAND;
    else if (strcasecmp(typeStr, "trailor") == 0)
        return TRAILOR;
    else if (strcasecmp(typeStr, "storage") == 0)
        return STORAGE;
    else {
        // Default type (should not occur if CSV is correctly formatted)
        return SLIP;
    }
}

// Get a string from a BoatType enum.
const char* getBoatTypeString(BoatType type) {
    switch(type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILOR: return "trailor";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}

// Find a boat by name (case-insensitive); returns pointer or NULL.
Boat* findBoatByName(const char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            return boats[i];
        }
    }
    return NULL;
}

// Load boats from a CSV file.
void loadBoatsFromFile(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening file for reading");
        return;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0)
            continue;
        
        // CSV format: name,length,type,extra,amountOwed
        char *token;
        token = strtok(line, ",");
        if (!token) continue;
        char name[128];
        strncpy(name, trimWhitespace(token), 127);
        name[127] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        float length = atof(trimWhitespace(token));

        token = strtok(NULL, ",");
        if (!token) continue;
        char typeStr[20];
        strncpy(typeStr, trimWhitespace(token), 19);
        typeStr[19] = '\0';
        BoatType type = getBoatType(typeStr);

        token = strtok(NULL, ",");
        if (!token) continue;
        char extraStr[50];
        strncpy(extraStr, trimWhitespace(token), 49);
        extraStr[49] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        float amountOwed = atof(trimWhitespace(token));

        if (boatCount >= MAX_BOATS) {
            printf("Maximum number of boats reached. Skipping extra entries.\n");
            break;
        }
        Boat *newBoat = malloc(sizeof(Boat));
        if (!newBoat) {
            perror("Memory allocation failed");
            fclose(fp);
            return;
        }
        strncpy(newBoat->name, name, 127);
        newBoat->name[127] = '\0';
        newBoat->length = length;
        newBoat->type = type;
        newBoat->amountOwed = amountOwed;
        // Store extra info based on boat type.
        switch(type) {
            case SLIP:
                newBoat->info.slip_number = atoi(extraStr);
                break;
            case LAND:
                newBoat->info.bay_letter = extraStr[0];
                break;
            case TRAILOR:
                strncpy(newBoat->info.trailer_license, extraStr, 19);
                newBoat->info.trailer_license[19] = '\0';
                break;
            case STORAGE:
                newBoat->info.storage_number = atoi(extraStr);
                break;
            default:
                break;
        }
        boats[boatCount++] = newBoat;
    }
    fclose(fp);
    // Keep the array sorted alphabetically by boat name.
    qsort(boats, boatCount, sizeof(Boat *), compareBoats);
}

// Save the boat data back to a CSV file.
void saveBoatsToFile(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening file for writing");
        return;
    }
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        char extra[50];
        switch(b->type) {
            case SLIP:
                sprintf(extra, "%d", b->info.slip_number);
                break;
            case LAND:
                sprintf(extra, "%c", b->info.bay_letter);
                break;
            case TRAILOR:
                sprintf(extra, "%s", b->info.trailer_license);
                break;
            case STORAGE:
                sprintf(extra, "%d", b->info.storage_number);
                break;
            default:
                strcpy(extra, "unknown");
                break;
        }
        // Write CSV: name,length,type,extra,amountOwed
        fprintf(fp, "%s,%.0f,%s,%s,%.2f\n", b->name, b->length, getBoatTypeString(b->type), extra, b->amountOwed);
    }
    fclose(fp);
}

// Print the boat inventory with aligned columns.
void printInventory(void) {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        switch(b->type) {
            case SLIP:
                printf("%-20s %3.0f' %-8s #%-3d   Owes $%8.2f\n",
                       b->name, b->length, "slip", b->info.slip_number, b->amountOwed);
                break;
            case LAND:
                printf("%-20s %3.0f' %-8s %-8c Owes $%8.2f\n",
                       b->name, b->length, "land", b->info.bay_letter, b->amountOwed);
                break;
            case TRAILOR:
                printf("%-20s %3.0f' %-8s %-8s Owes $%8.2f\n",
                       b->name, b->length, "trailor", b->info.trailer_license, b->amountOwed);
                break;
            case STORAGE:
                printf("%-20s %3.0f' %-8s #%-3d   Owes $%8.2f\n",
                       b->name, b->length, "storage", b->info.storage_number, b->amountOwed);
                break;
            default:
                printf("%-20s %3.0f' %-8s   Owes $%8.2f\n",
                       b->name, b->length, "unknown", b->amountOwed);
                break;
        }
    }
}

// Add a boat using a CSV-formatted string.
void addBoatFromCSV(const char *csvLine) {
    char line[MAX_LINE_LENGTH];
    strncpy(line, csvLine, MAX_LINE_LENGTH - 1);
    line[MAX_LINE_LENGTH - 1] = '\0';

    // CSV format: name,length,type,extra,amountOwed
    char *token;
    token = strtok(line, ",");
    if (!token) return;
    char name[128];
    strncpy(name, trimWhitespace(token), 127);
    name[127] = '\0';

    token = strtok(NULL, ",");
    if (!token) return;
    float length = atof(trimWhitespace(token));

    token = strtok(NULL, ",");
    if (!token) return;
    char typeStr[20];
    strncpy(typeStr, trimWhitespace(token), 19);
    typeStr[19] = '\0';
    BoatType type = getBoatType(typeStr);

    token = strtok(NULL, ",");
    if (!token) return;
    char extraStr[50];
    strncpy(extraStr, trimWhitespace(token), 49);
    extraStr[49] = '\0';

    token = strtok(NULL, ",");
    if (!token) return;
    float amountOwed = atof(trimWhitespace(token));

    if (boatCount >= MAX_BOATS) {
        printf("Cannot add boat: maximum number of boats reached.\n");
        return;
    }
    Boat *newBoat = malloc(sizeof(Boat));
    if (!newBoat) {
        perror("Memory allocation failed");
        return;
    }
    strncpy(newBoat->name, name, 127);
    newBoat->name[127] = '\0';
    newBoat->length = length;
    newBoat->type = type;
    newBoat->amountOwed = amountOwed;
    switch(type) {
        case SLIP:
            newBoat->info.slip_number = atoi(extraStr);
            break;
        case LAND:
            newBoat->info.bay_letter = extraStr[0];
            break;
        case TRAILOR:
            strncpy(newBoat->info.trailer_license, extraStr, 19);
            newBoat->info.trailer_license[19] = '\0';
            break;
        case STORAGE:
            newBoat->info.storage_number = atoi(extraStr);
            break;
        default:
            break;
    }
    boats[boatCount++] = newBoat;
    qsort(boats, boatCount, sizeof(Boat *), compareBoats);
    printf("Boat added successfully.\n");
}

// Remove a boat by name (case-insensitive).
void removeBoatByName(const char *name) {
    int found = 0;
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            free(boats[i]);
            for (int j = i; j < boatCount - 1; j++) {
                boats[j] = boats[j + 1];
            }
            boatCount--;
            found = 1;
            printf("Boat removed successfully.\n");
            break;
        }
    }
    if (!found)
        printf("No boat with that name.\n");
}

// Update monthly charges for all boats.
void updateMonthlyCharges(void) {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        float charge = 0.0;
        switch(b->type) {
            case SLIP:
                charge = b->length * 12.50;
                break;
            case LAND:
                charge = b->length * 14.00;
                break;
            case TRAILOR:
                charge = b->length * 25.00;
                break;
            case STORAGE:
                charge = b->length * 11.20;
                break;
            default:
                break;
        }
        b->amountOwed += charge;
    }
    printf("Monthly charges updated.\n");
}

// Main program with menu loop.
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }

    // Load boat data from the CSV file.
    loadBoatsFromFile(argv[1]);

    char option;
    char inputBuffer[256];
    while (1) {
        printf("\n(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) == NULL)
            break;
        if (sscanf(inputBuffer, " %c", &option) != 1)
            continue;
        option = tolower(option);

        if (option == 'i') {
            printInventory();
        } else if (option == 'a') {
            printf("Please enter the boat data in CSV format: ");
            if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
                inputBuffer[strcspn(inputBuffer, "\n")] = '\0';
                addBoatFromCSV(inputBuffer);
            }
        } else if (option == 'r') {
            printf("Please enter the boat name: ");
            if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
                inputBuffer[strcspn(inputBuffer, "\n")] = '\0';
                removeBoatByName(inputBuffer);
            }
        } else if (option == 'p') {
            printf("Please enter the boat name: ");
            if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
                inputBuffer[strcspn(inputBuffer, "\n")] = '\0';
                Boat *boat = findBoatByName(inputBuffer);
                if (boat == NULL) {
                    printf("No boat with that name.\n");
                } else {
                    printf("Please enter the amount to be paid: ");
                    if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
                        float payment = atof(inputBuffer);
                        if (payment > boat->amountOwed) {
                            printf("That is more than the amount owed, $%.2f\n", boat->amountOwed);
                        } else {
                            boat->amountOwed -= payment;
                            printf("Payment accepted. New amount owed: $%.2f\n", boat->amountOwed);
                        }
                    }
                }
            }
        } else if (option == 'm') {
            updateMonthlyCharges();
        } else if (option == 'x') {
            // Save data and exit.
            saveBoatsToFile(argv[1]);
            printf("\nExiting the Boat Management System\n");
            break;
        } else {
            printf("Invalid option %c\n", option);
        }
    }

    // Free all allocated memory.
    for (int i = 0; i < boatCount; i++) {
        free(boats[i]);
    }

    return 0;
}
