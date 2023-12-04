#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <regex>
#include <random>
#include <vector> // Include the vector header

float random_pri() {
    // Create a random number generator engine
    std::random_device rd;   // Use a hardware random device if available
    std::mt19937 gen(rd()); // Mersenne Twister engine for 32-bit integers

    // Define a uniform distribution for floating-point values between 0.0 and 1.0
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    // Generate and return a random value
    return distribution(gen);
}

int main() {
    int numIterations;
    std::vector<double> CV_a_5G;
    std::vector<double> UP_a_5G;
    std::vector<double> SE_a_5G;

    // Prompt the user for the number of iterations
    std::cout << "Enter the number of iterations: ";
    std::cin >> numIterations;

    // The command to run
    const char* command = "./ns3 run scratch/5GsimNS3/5Gmain.cc";

    // Create a text file to save the RSSI, SE, and UE position values
    std::ofstream outputFile("rssi_se_position.txt");

    for (int i = 0; i < numIterations; ++i) {
        std::cout << "Running iteration " << (i + 1) << "..." << std::endl;

        // Execute the command using the system function and capture the output
        FILE* pipe = popen(command, "r");
        if (!pipe) {
            std::cerr << "Error executing command." << std::endl;
            return 1;
        }

        // Read the command output line by line
        std::string line;
        char buffer[128]; // Declare a buffer to read lines into
        double rssiValue = 0.0; // Initialize RSSI value
        double seValue = 0.0;   // Initialize SE value
        std::string uePosition; // Initialize UE position as a string

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            line = buffer;

            // Check for lines containing RSSI, SE, and UE position values
            if (line.find("RSSI: ") != std::string::npos) {
                size_t rssiPos = line.find("RSSI: ") + 6;
                rssiValue = std::stod(line.substr(rssiPos, line.find(" dBm", rssiPos) - rssiPos));
            } else if (line.find("SE: ") != std::string::npos) {
                size_t sePos = line.find("SE: ") + 4;
                seValue = std::stod(line.substr(sePos, line.find(" bps/Hz", sePos) - sePos));
            } else if (line.find("Position(") != std::string::npos) {
                size_t startPos = line.find("Position(") + 9;
                size_t endPos = line.find(")", startPos);
                uePosition = line.substr(startPos, endPos - startPos);
            }
        }

        pclose(pipe);

        double UP = random_pri();

        ///////////////////// changing CV and SE to [0,1] //////////////////////////////////////////
        double maxCV = 130.0; // Maximum CV value
        double CV_5G = ((rssiValue + 120) / maxCV);
        printf("CV_5G (check) = %f\n",CV_5G);

        double maxSE = 10.0; // Maximum SE value
        double SE_5G = ((seValue / maxSE));
        printf("SE_5G (check) = %f\n",SE_5G);

        CV_a_5G.push_back(CV_5G);
        UP_a_5G.push_back(UP);
        SE_a_5G.push_back(SE_5G);

        std::cout << "RSSI: " << rssiValue << " dBm, SE: " << seValue << " bps/Hz, UE Position: " << uePosition << ", User priority = " << UP << std::endl;

        // Write the captured values to the output file
        outputFile << "Iteration " << (i + 1) << ": RSSI = " << rssiValue << " dBm, SE = " << seValue << " bps/Hz, UE Position = " << uePosition << ", User priority = " << UP << std::endl;
    }

    std::ofstream dataFilewifiall("data_5G-raw.txt");
    // Check if the file stream is open/valid.
    if (!dataFilewifiall) {
        std::cerr << "Cannot open the output file!" << std::endl;
        return 1;
    }
    for (size_t i = 0; i < numIterations; ++i) {
        dataFilewifiall << CV_a_5G[i] << " " << UP_a_5G[i] << " " << SE_a_5G[i] << "\n";
    }

    dataFilewifiall.close();

    // Close the output file
    outputFile.close();
    printf("------ Saving 5G data -------\n");
    return 0;
}
