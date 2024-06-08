#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "gpio.h"

UART_HandleTypeDef huart2;

// Function prototypes
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

void open_garage_door(void);
void close_garage_door(void);
int is_car_entered(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    char uart_buffer[100];
    char plate_status[10];
    
    while (1)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)"CAPTURE_IMAGE\n", 14, HAL_MAX_DELAY);
        HAL_UART_Receive(&huart2, (uint8_t*)uart_buffer, sizeof(uart_buffer), HAL_MAX_DELAY);

        if (sscanf(uart_buffer, "PLATE_%s", plate_status) == 1)
        {
            if (strcmp(plate_status, "OK") == 0)
            {
                open_garage_door();
                
                // Wait for the car to enter
                while (!is_car_entered())
                {
                    HAL_Delay(100);  // Check every 100 milliseconds
                }
                
                close_garage_door();
            }
        }

        HAL_Delay(1000);  // Wait for a while before capturing the next image
    }
}

void open_garage_door(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
}

void close_garage_door(void)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

int is_car_entered(void)
{
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET;
}

void SystemClock_Config(void)
{
    // System Clock Configuration code here
}

static void MX_GPIO_Init(void)
{
    // GPIO Initialization code here

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure GPIO pin for the relay
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure GPIO pin for the sensor
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        // Initialization Error
        Error_Handler();
    }
}

//code esp32
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// Replace with your network credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Replace with your ANPR server address
const char* serverUrl = "http://your_anpr_server/analyze";

ESP8266WebServer server(80);

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_POST, handleCaptureImage);
    server.begin();
}

void loop()
{
    server.handleClient();
}

void handleCaptureImage()
{
    if (captureImage())
    {
        sendImageToServer();
    }
}

bool captureImage()
{
    // Camera capture code here
    // Return true if successful, false otherwise
    return true;
}

void sendImageToServer()
{
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Construct JSON payload with the captured image
    StaticJsonDocument<200> doc;
    doc["image"] = "base64_encoded_image_data";
    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println(response);
        Serial1.println(response);  // Send response to STM32 via UART
    }
    else
    {
        Serial.println("Error in sending POST");
    }

    http.end();
}
