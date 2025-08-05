/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "fonts.h"
#include "assets.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFF_SIZE 4096

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define USER_BUTTON_PIN GPIO_PIN_0
#define USER_BUTTON_PORT GPIOA
#define DEBOUNCE_DELAY 50
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//void myprintf(const char *fmt, ...) {
//  static char buffer[256];
//  va_list args;
//  va_start(args, fmt);
//  vsnprintf(buffer, sizeof(buffer), fmt, args);
//  va_end(args);
//
//  int len = strlen(buffer);
////  HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, -1);
//
//  ILI9341_WriteString(40, 40, buffer, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
//
//}
bool print_once = false;


struct Character {
	int16_t x;
	int16_t y;
	int16_t width;
	int16_t height;
	int16_t x_distance_between_frame;
	int16_t y_distance_between_frame;
	float y_velocity;
};

struct Enemy {
	int16_t x;
	int16_t y;
	int16_t width;
	int16_t height;
	int16_t x_distance_between_frame;
	int16_t y_distance_between_frame;
	int16_t start;
	int16_t end;
	float y_velocity;
	bool dir;
	bool died;
	bool active;
};


struct Object {
	int16_t x;
	int16_t y;
	int16_t prev_x;
	int16_t prev_y;
	int16_t width;
	int16_t height;
	uint16_t* frame;
	bool collidable;
	bool redraw;
};


struct Object objects[20];
uint16_t mario_final[34*32];
uint16_t bowser_final[64*64];
uint8_t num_objects = 0;

uint8_t prev_mario_x = 0;
uint8_t mario_x = 0;
uint8_t prev_mario_y = 0;
uint8_t mario_y = 0;
volatile uint32_t last_press = 0;
bool moved = false;
bool idle = false;
bool dir = false;

struct Character prev_mario = { 80, 176, 26, 32, 0, 0, 0 };
struct Character mario = { 80, 176, 26, 32, 0, 0, 0 };

struct Enemy prev_bowser = { 240, 144, 64, 64, 0, 0, 80, 200, 0, false, true};
struct Enemy bowser = { 240, 144, 64, 64, 0, 0, 80, 200, 0, false, false, true};

//struct Enemy prev_donkey = { 280, 176, 32, 32, 0, 0, 80, 200, 0, false, true};
//struct Enemy donkey = { 280, 176, 32, 32, 0, 0, 80, 200, 0, false, true};
//struct Enemy prev_bowser = { 280, 176, 32, 32, 0, 0, 80, 200, 0, false, true};
//struct Enemy bowser = { 280, 176, 32, 32, 0, 0, 80, 200, 0, false, true};
//
//
//struct Enemy* enemies[3] = { &goomba, &donkey, &bowser };
struct Enemy* enemies[3] = { &bowser };

unsigned char audio_buff[BUFF_SIZE];
unsigned char yahoo_buff[BUFF_SIZE];
uint16_t buff_pointer = 0;
uint16_t yahoo_buff_pointer = 0;

FATFS FatFs; 	//Fatfs handle
FIL fil; 		//File handle
FIL fil_yahoo; 		//File handle
FRESULT fres; //Result after operations

int num_iter = 0;
int num_not_equal_iter = 0;
bool do_once = false;
bool is_jumping = false;
bool reset_jump_audio = false;
uint32_t jump_timer;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
        // Get next sample from buffer
		uint8_t sampled_8bit_pwm_val = audio_buff[buff_pointer++];
		if (is_jumping && HAL_GetTick() - jump_timer < 500) {
			sampled_8bit_pwm_val = (3 * sampled_8bit_pwm_val + yahoo_buff[yahoo_buff_pointer++]) / 4;
			reset_jump_audio = false;
		}
		else {
			if (!reset_jump_audio) {
				reset_jump_audio = true;
				f_lseek(&fil_yahoo, 0x2c);
				yahoo_buff_pointer = 0;
				int bytesRead = 0;
				FRESULT res = f_read(&fil_yahoo, yahoo_buff, BUFF_SIZE, &bytesRead);
			}
		}

        // Scale 16-bit to 8-bit unsigned for PWM (period=255)
        // basically take the 16 bit value, shift by 32768 to make it
        // start from 0. Then multiply by 255
//        uint8_t pwm_value = ((uint16_t)(sample16 + 32768) * 255) >> 16;

//		if (!do_once) {
//			char hex_str[16];
//			sprintf(hex_str, "0x%02X", sampled_8bit_pwm_val);
//			ILI9341_WriteString(80, 0, hex_str, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
//			do_once = true;
//		}
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, sampled_8bit_pwm_val);

        // Refill buffer if needed
        if (buff_pointer >= BUFF_SIZE) {
        	num_iter++;
//			char hex_str[16];
//			sprintf(hex_str, "0x%02X", pwm_value);
//			ILI9341_WriteString(80, 0, hex_str, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
            // refill buffer with f_read (non-blocking, or with a flag in main loop)
            // or set a "need_refill" flag to be handled in main()
        	int bytesRead = 0;
        	FRESULT res = f_read(&fil, audio_buff, BUFF_SIZE, &bytesRead);
        	if (bytesRead < BUFF_SIZE) {
        		f_lseek(&fil, 0x2C);
        	}

        	buff_pointer = 0;
        }

        if (yahoo_buff_pointer >= BUFF_SIZE) {
        	int bytesRead = 0;
        	FRESULT res = f_read(&fil_yahoo, yahoo_buff, BUFF_SIZE, &bytesRead);
        	if (bytesRead < BUFF_SIZE) {
				f_lseek(&fil_yahoo, 0x2c);
        	}

        	yahoo_buff_pointer = 0;
        }
	}
}

void init() {
    ILI9341_Unselect();
//    ILI9341_TouchUnselect();
    ILI9341_Init();
}

// turn left if walking left
void turn_mario(uint8_t width, uint8_t height) {
	if (mario.x_distance_between_frame < 0 || (mario.x_distance_between_frame == 0 && dir == true)) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width / 2; col++) {
                int left  = row * width + col;
                int right = row * width + (width - 1 - col);

                // Swap 16-bit pixels
                uint16_t temp = mario_final[left];
                mario_final[left] = mario_final[right];
                mario_final[right] = temp;
            }
        }

		dir = true;
	}
	else if (mario.x_distance_between_frame > 0) {
		dir = false;
	}

}

void turn_enemy(uint8_t width, uint8_t height, uint16_t* enemy_final, struct Enemy enemy) {
	if (enemy.x_distance_between_frame < 0 || (enemy.x_distance_between_frame == 0 && dir == true)) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width / 2; col++) {
                int left  = row * width + col;
                int right = row * width + (width - 1 - col);

                // Swap 16-bit pixels
                uint16_t temp = enemy_final[left];
                enemy_final[left] = enemy_final[right];
                enemy_final[right] = temp;
            }
        }

		dir = true;
	}
	else if (enemy.x_distance_between_frame > 0){
		dir = false;
	}

}

void byte_order_switch(uint16_t size, uint8_t arr[]) {

	for (uint16_t i = 0 ; i < size; i+=2) {
		uint16_t temp = arr[i];
		arr[i] = arr[i+1];
		arr[i+1] = temp;
	}
}


bool collision_detection(struct Character mario, struct Object object) {
	uint16_t Ax = mario.x;
	uint16_t Ay = mario.y;

	uint16_t Bx = object.x;
	uint16_t By = object.y;

	uint16_t A_left   = Ax;
	uint16_t A_right  = Ax + mario.width;
	uint16_t A_top    = Ay;
	uint16_t A_bottom = Ay + mario.height;

	uint16_t B_left   = Bx;
	uint16_t B_right  = Bx + object.width;
	uint16_t B_top    = By;
	uint16_t B_bottom = By + object.height;

	bool x_overlap = (A_right >  B_left) && (B_right >  A_left);
	bool y_overlap = (A_bottom > B_top ) && (B_bottom > A_top );
	return x_overlap && y_overlap;
}

bool collision_detection_enemies() {

	for (int i = 0; i < 1; i++) {
//
//	int16_t x;
//	int16_t y;
//	int16_t prev_x;
//	int16_t prev_y;
//	int16_t width;
//	int16_t height;
//	uint16_t* frame;
//	bool collidable;
//	bool redraw;

		struct Object enemy_object = { enemies[i]->x, enemies[i]->y, 0, 0, enemies[i]->width, enemies[i]->height, bowser_final, true, false };

		if (enemies[i]->died || !collision_detection(mario,enemy_object))
			continue;


		// Landing from above (moving downward)
		if (mario.y_velocity > 0 &&
			prev_mario.y + prev_mario.height <= enemies[i]->y) {

			mario.y = enemies[i]->y - mario.height - 40;
			if (i == 0) {
				enemies[i]->died = true;
			}
			return true;
		}
		// Hit head from below (moving upward)
		else if (mario.y_velocity < 0 &&
				 prev_mario.y >= enemies[i]->y + enemies[i]->height) {

			mario.y = enemies[i]->y + enemies[i]->height;
			mario.y_velocity = 0.1;  // Start falling
			return true;
		}
		// Hit from left
		else if (prev_mario.x + prev_mario.width <= enemies[i]->x) {
			mario.x = enemies[i]->x - mario.width;
			mario.x_distance_between_frame = 0;
			return true;
		}
		// Hit from right
		else if (prev_mario.x >= enemies[i]->x + enemies[i]->width) {
			mario.x = enemies[i]->x + enemies[i]->width;
			mario.x_distance_between_frame = 0;
			return true;
		}
	}

	return false;

}



bool standing_on(struct Character mario, struct Object object) {
    uint16_t Ax = mario.x;
    uint16_t Ay = mario.y;

    uint16_t Bx = object.x;
    uint16_t By = object.y;

    uint16_t A_left   = Ax;
    uint16_t A_right  = Ax + mario.width;
    uint16_t A_bottom = Ay + mario.height;

    uint16_t B_left   = Bx;
    uint16_t B_right  = Bx + object.width;
    uint16_t B_top    = By;

    // Check if Mario is horizontally overlapping with the object
    bool x_overlap = (A_right > B_left) && (A_left < B_right);

    // Check if Mario's bottom is within 2 pixels of the object's top
    // This gives tolerance for floating-point precision issues
    bool y_standing = (A_bottom >= B_top - 1) && (A_bottom <= B_top + 1);

    return x_overlap && y_standing;
}

bool isTouchingGround() {
    bool onGround = false;
    bool collision_detected = false;

    for (uint8_t i = 0; i < num_objects; i++) {
        if (!objects[i].collidable) {
			if (collision_detection(prev_mario, objects[i]) || collision_detection(mario, objects[i])) {
				ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);
				objects[i].redraw = true;
				break;
			} else if (objects[i].redraw) {
				ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);
				objects[i].redraw = false;
			}
        continue;
        }

        if (!collision_detection(mario, objects[i]))
            continue;

        collision_detected = true;

        // Landing from above (moving downward)
        if (mario.y_velocity > 0 &&
            prev_mario.y + prev_mario.height <= objects[i].y) {

            mario.y = objects[i].y - mario.height;
            mario.y_velocity = 0;
            onGround = true;
        }
        // Hit head from below (moving upward)
        else if (mario.y_velocity < 0 &&
                 prev_mario.y >= objects[i].y + objects[i].height) {

            mario.y = objects[i].y + objects[i].height;
            mario.y_velocity = 0.1;  // Start falling
        }
        // Hit from left
        else if (prev_mario.x + prev_mario.width <= objects[i].x) {
            mario.x = objects[i].x - mario.width;
            mario.x_distance_between_frame = 0;
        }
        // Hit from right
        else if (prev_mario.x >= objects[i].x + objects[i].width) {
            mario.x = objects[i].x + objects[i].width;
            mario.x_distance_between_frame = 0;
        }

        break;  // Handle only one collision per frame
    }

    // FIXED GRAVITY LOGIC:
    // Only apply gravity if:
    // 1. Mario is not currently on ground from collision detection
    // 2. Mario is not standing on any platform
    // 3. Mario has zero velocity (not already falling/jumping)
    // 4. Mario is not at the bottom boundary

    if (!onGround && mario.y_velocity == 0 && mario.y < 176) {
        bool is_standing_on_something = false;

        for (uint8_t i = 0; i < num_objects; i++) {
            if (objects[i].collidable && standing_on(mario, objects[i])) {
                is_standing_on_something = true;
                break;
            }
        }

        // Only apply gravity if not standing on anything
        if (!is_standing_on_something) {
            mario.y_velocity = 0.1;
        }
    }

    return onGround;
}



void cleanMarioBackground(uint16_t* frame, uint16_t* buffer, uint16_t width, uint16_t height, uint16_t buffsize) {
    for (uint16_t i = 0; i < buffsize; i++) {
        buffer[i] = ILI9341_CYAN;  // Initialize everything to cyan
    }
	for (uint16_t i = 0; i < width * height; i++) {
		if (frame[i] != 0){
			buffer[i] = frame[i];
		}
	}
}
//
//
//void cleanMarioBackground(uint16_t* frame, uint16_t* buffer, uint16_t width, uint16_t height) {
//	for (uint16_t i = 0; i < width; i++) {
//		for (uint16_t j = 0; j < height; j++) {
//			uint16_t idx = i * width + j;
//
//			uint16_t color = frame[idx];
//			uint8_t r = (color >> 11) & 0x1F;
//			uint8_t g = (color >> 5) & 0x3F;
//			uint8_t b = color & 0x1F;
//
//			// Thresholds: adjust to your liking!
//			if (r < 4 && g < 8 && b < 4) {
//				buffer[idx] = ILI9341_CYAN; // or actual background pixel
//			} else {
//				buffer[idx] = color;
//			}
//		}
//	}
//}

void WaitForButtonPress(void) {
    uint8_t button_pressed = 0;
    uint32_t press_time = 0;

    // Wait for button press (falling edge - goes from 1 to 0)
    while (1) {
        if (HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN) == 0 && !button_pressed) {
            press_time = HAL_GetTick();
            button_pressed = 1;
        }

        // Wait for button release with debounce (goes back to 1)
        if (button_pressed && HAL_GPIO_ReadPin(USER_BUTTON_PORT, USER_BUTTON_PIN) == 1) {
            if ((HAL_GetTick() - press_time) > DEBOUNCE_DELAY) {
                break;  // Button press complete - exit the while loop
            }
        }

        HAL_Delay(10);  // Small delay to prevent excessive polling
    }
}

void hard_reset_system() {
    // Disable all SPI
    HAL_SPI_DeInit(&hspi2);

    // Reset all GPIO

    HAL_GPIO_DeInit(SD_CS_GPIO_Port, SD_CS_Pin);

    // Wait for everything to discharge
    HAL_Delay(100);

    // Reinitialize everything
    MX_GPIO_Init();
    MX_SPI2_Init();

    HAL_Delay(50);

}


uint8_t map = 1;

void draw_map_1() {
	ILI9341_FillScreen(ILI9341_CYAN);
	for (int i = 0; i < 10; i++) {
		objects[i].x = i * 32;
		objects[i].prev_x = i * 32;
		objects[i].y = 208;
		objects[i].prev_y = 208;
		objects[i].width = 32;
		objects[i].height = 32;
		objects[i].frame = ground;
		objects[i].collidable = true;


		ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);
	}

	objects[10].x = 160;
	objects[10].prev_x = 160;
	objects[10].y = 126;
	objects[10].prev_y = 160;
	objects[10].width = 32;
	objects[10].height = 32;
	objects[10].frame = brick;
	objects[10].collidable = true;
	objects[10].redraw = false;

	objects[15].x = 230;
	objects[15].prev_x = 230;
	objects[15].y = 40;
	objects[15].prev_y = 230;
	objects[15].width = 64;
	objects[15].height = 48;
	objects[15].frame = cloud;
	objects[15].collidable = false;
	objects[15].redraw = false;


	for (int i = 10; i < 16; i++)
		ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);

	num_objects = 16;


//		// Test SPI communication first
//		DSTATUS init_status = USER_SPI_initialize(0);
//		char debug_msg[64];
//		snprintf(debug_msg, sizeof(debug_msg), "SPI Init: %d", init_status);
//		ILI9341_WriteString(40, 20, debug_msg, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
//		HAL_Delay(100);
//
//		// Check if SPI handle is working
//		if (SD_SPI_HANDLE.Instance == NULL) {
//		    ILI9341_WriteString(40, 60, "SPI NULL!", Font_11x18, ILI9341_RED, ILI9341_WHITE);
//		    while(1);
//		}
//
//		// Test basic SPI transfer
//		uint8_t test_data = 0xFF;
//		uint8_t received = 0;
//		HAL_StatusTypeDef spi_status = HAL_SPI_TransmitReceive(&SD_SPI_HANDLE, &test_data, &received, 1, 1000);
//		snprintf(debug_msg, sizeof(debug_msg), "SPI Test: %d, RX: 0x%02X", spi_status, received);
//		ILI9341_WriteString(40, 80, debug_msg, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

	// test
	HAL_Delay(2000); // Give time to read debug info

}

void draw_bowser() {
	static uint8_t frame_num = 0;
	static uint16_t* frames[4] = { bowser_1, bowser_2, bowser_3, bowser_4 };
	if (frame_num > 3)
		frame_num = 0;
	uint16_t* bowser_frame =  frames[frame_num];

	ILI9341_FillRectangle(prev_bowser.x, prev_bowser.y, prev_bowser.width, prev_bowser.height, ILI9341_CYAN);
	cleanMarioBackground(bowser_frame, bowser_final, 64, 64, 64*64);
	if (bowser.x > prev_bowser.x) {
		turn_enemy(bowser.width, bowser.height, bowser_final, bowser);
	}

	ILI9341_DrawImage(bowser.x, bowser.y, bowser.width, bowser.height, bowser_final);
	frame_num++;
}

void drawScene(uint8_t map_num) {
	if (map_num == 1) {

		draw_map_1();
	}
}



//void fill_background(uint16_t img, uint16_t output, uint8_t width, uint8_t height) {
//	for (int i = 0; i < width; i++) {
//		for (int j = 0; j < height; j++)
//	}
//}

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
//    if (GPIO_Pin == RIGHT_BUTTON_Pin) {
//    	uint32_t now = HAL_GetTick();
//    	if (now - last_press > 100) {
//    		prev_mario_x = mario_x;
//        	mario_x += 3;
//        	last_press = now;
//        	moved = true;
//    	}
//    }
//}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_FATFS_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
//  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
//  HAL_GPIO_WritePin(SD_Power_GPIO_Port, SD_Power_Pin, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  init();

//  ILI9341_FillScreen(ILI9341_CYAN);


  uint32_t last_tick = HAL_GetTick();
  uint32_t move_tick = HAL_GetTick();
  uint32_t walk_tick = HAL_GetTick();
  uint32_t read_pin_tick = HAL_GetTick();
  uint8_t curr_walk_frame = 0;
  uint16_t* walk_frames[] = { mario_walk_1, mario_walk_2, mario_walk_3 };
  uint8_t walk_frames_size[] = { 32, 32, 24, 32, 28, 32 };
  uint8_t text_height = 0;
  bool idle = false;

  uint32_t now = HAL_GetTick();
  uint32_t frame_count = 0;

//  hard_reset_system();
  HAL_Delay(50);
  ILI9341_WriteString(40, 20, "Starting Mount!", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
  HAL_Delay(50);


//	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, SET);
	// Now try mount
	  //Open the file system
	  fres = f_mount(&FatFs, "", 1); //1=mount now
	  while (fres != FR_OK) {
		char msg[32];
		snprintf(msg, sizeof(msg), "mount err %u", (unsigned)fres);
		HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, RESET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, SET);
		HAL_Delay(10);
		ILI9341_WriteString(40, 60, msg, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
		HAL_Delay(100);
		fres = f_mount(&FatFs, "", 1); //1=mount now
		ILI9341_WriteString(40, 60, "Retrying: please wait!", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
	  }

	  HAL_Delay(10);
	  ILI9341_WriteString(0, 280, "SD Card Mounted", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
	  HAL_Delay(5000);
	  DWORD free_clusters, free_sectors, total_sectors;

	  FATFS* getFreeFs;

	  fres = f_getfree("", &free_clusters, &getFreeFs);
	  if (fres != FR_OK) {
		  ILI9341_WriteString(40, 40, "step 2", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
//			myprintf("f_getfree error (%i)\r\n", fres);
		while(1);
	  }

	  fres = f_open(&fil, "mario.wav", FA_READ);
	  if (fres != FR_OK) {
		  ILI9341_WriteString(40, 40, "step 3", Font_11x18, ILI9341_BLACK, ILI9341_WHITE);
	  }
//			myprintf("f_open error (%i)\r\n");
	  fres = f_open(&fil_yahoo, "yahoo.wav", FA_READ);

	  //Read first 44 bytes of the wav file
//	  BYTE readBuff[44];
//      FRESULT res = f_read(&fil, readBuff, 44, &numRead);
	  f_lseek(&fil, 0x2C);
	  f_lseek(&fil_yahoo, 0x55);
      HAL_Delay(10);

	  int numRead = 0;
      FRESULT res = f_read(&fil, audio_buff, BUFF_SIZE, &numRead);
      res = f_read(&fil_yahoo, yahoo_buff, BUFF_SIZE, &numRead);
	  HAL_Delay(300);
	  drawScene(1);

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);     // PWM out
  HAL_TIM_Base_Start_IT(&htim3);
  uint16_t* curr_goomba = goomba_1;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (mario.x < 0) mario.x = 0;
    if (mario.x > 320 - mario.width) mario.x = 320 - mario.width;
	uint32_t now = HAL_GetTick();


//	if (!moved && !idle) {
//		mario.width = 26;
//		mario.height = 32;
//
//		mario.x += mario.x_distance_between_frame;
//		mario.x_distance_between_frame = 0;
//
//		ILI9341_FillRectangle(prev_mario.x, prev_mario.y, prev_mario.width, prev_mario.height, ILI9341_CYAN);
////		cleanMarioBackground(mario_idle, mario_final, mario.width, mario.height);
////		ILI9341_DrawImage(mario.x, mario.y, mario.width, mario.height, mario_final);
//		ILI9341_DrawImage(mario.x, mario.y, mario.width, mario.height, mario_idle);
//		idle = true;
//		prev_mario = mario;
//	}

	// DRAW LOOP
	if (now - walk_tick >= 100) {
//		if (!goomba.died) {
//			prev_goomba = goomba;
//			if (!goomba.dir) {
//				goomba.x -= 2;
//				if (goomba.x <= goomba.start) {
//					goomba.dir = true;
//				}
//			} else {
//				goomba.x += 2;
//				if (goomba.x >= goomba.end) {
//					goomba.dir = false;
//				}
//		}
//		} else if (goomba.active) {
//			goomba.active = false;
//			prev_goomba = goomba;
//			ILI9341_FillRectangle(prev_goomba.x, prev_goomba.y, prev_goomba.width, prev_goomba.height, ILI9341_CYAN);
//		}

		uint16_t* frame;
		// no movement happened
		if (!mario.x_distance_between_frame && (!mario.y_velocity)) {
			frame = mario_idle;
			mario.width = 26;
			mario.height = 32;
		}
		else {
			mario.x += mario.x_distance_between_frame;
//			for (int i = 0; i < num_objects; i++) {
//				objects[i].prev_x = objects[i].x;
//				objects[i].x -= mario.x_distance_between_frame;
//			}

			if (mario.y_velocity) {
					mario.y_velocity += 0.10 * 550;
					float y_traveled = mario.y_velocity * 0.10;

					mario.y += y_traveled;

//					if (mario.y > 176) {
//						mario.y = 176;
//						mario.y_velocity = 0;
//					}
			}

			if (mario.y_velocity) {
				frame = mario_jump;
				mario.width = 34;
				mario.height = 32;
			}
			else {
				curr_walk_frame++;
				curr_walk_frame %= 3;
				frame = walk_frames[curr_walk_frame];
				mario.width = walk_frames_size[curr_walk_frame * 2];
				mario.height = walk_frames_size[curr_walk_frame * 2 + 1];
			}
//			uint8_t prev_frame = (curr_walk_frame == 0) ? 2 : curr_walk_frame - 1;
//			uint8_t prev_frame_x = walk_frames_size[prev_frame * 2];
//			uint8_t prev_frame_y = walk_frames_size[prev_frame * 2 + 1];

//			char buf_xy[32];
//			char buf_vel[16];

//			sprintf(buf_xy, "(%d)", (int)mario.y_velocity);
//			ILI9341_WriteString(180, 200, buf_xy, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);

//			sprintf(buf_vel, "%f", mario.y_velocity);
//			ILI9341_WriteString(180, 220, buf_vel, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
		}

		collision_detection_enemies();
		bool touching_ground = isTouchingGround();
		// if mario isn't idle or he is but its the first frame where he is idle, then draw
		// otherwise skip to optimize speed.
		if (frame != mario_idle || (frame == mario_idle && !idle)) {

			if (!idle && frame == mario_idle) {
				// prevent another print while he's idle
				idle = true;
			}

			if (idle && frame != mario_idle) {
				idle = false;
			}


			ILI9341_FillRectangle(prev_mario.x, prev_mario.y, prev_mario.width, prev_mario.height, ILI9341_CYAN);
	//		ILI9341_DrawImage(mario.x, mario.y, mario.width, mario.height, frame);
			cleanMarioBackground(frame, mario_final, mario.width, mario.height, 34*32);
			//			for (int i = 0; i < num_objects; i++) {
//				if (objects[i].redraw) {
//					ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);
//					objects[i].redraw = false;
//				}
//			}
			turn_mario(mario.width, mario.height);
//			for (int i = 0; i < num_objects; i++) {
//				ILI9341_FillRectangle(objects[i].prev_x, objects[i].y, objects[i].width, objects[i].height, ILI9341_CYAN);
//			}
//
//			for (int i = 0; i < num_objects; i++) {
//				if (objects[i].x >= 0 && objects[i].x < 320 || objects[i].y >= 0 || objects[i].y < 240) {
//					ILI9341_DrawImage(objects[i].x, objects[i].y, objects[i].width, objects[i].height, objects[i].frame);
//				}
//			}
			ILI9341_DrawImage(mario.x, mario.y, mario.width, mario.height, mario_final);
			draw_bowser();
	//		ILI9341_DrawImage(mario.x, mario.y, mario.width, mario.height, frame);
		}

//		if (goomba.active) {
//			curr_goomba = (curr_goomba == goomba_1) ? goomba_2 : goomba_1;
//			cleanMarioBackground(curr_goomba, goomba_final, goomba.width, goomba.height);
//			ILI9341_FillRectangle(prev_goomba.x, prev_goomba.y, prev_goomba.width, prev_goomba.height, ILI9341_CYAN);
//			ILI9341_DrawImage(goomba.x, goomba.y, goomba.width, goomba.height, goomba_final);
//		}

		prev_mario = mario;
		mario.x_distance_between_frame = 0;
		walk_tick = now;
	}

	frame_count++;

//	if (now - last_tick >= 500) {
//		// 1 second has passed!
//		ILI9341_FillRectangle(0, text_height, 100, 20, ILI9341_RED);
//		text_height+= 2;
//		ILI9341_WriteString(10, text_height, "FPS: ", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
//		char buf[16];
//		sprintf(buf, "%lu", frame_count);
//		ILI9341_WriteString(0, text_height, buf, Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
//		frame_count = 0;
//		last_tick += 1000;  // Move up by 1 second, avoids drift
//   }

//	if (frame_count > 30) {
//		HAL_Delay(10);
//	}
	// forward
	if (now - read_pin_tick >= 50) {
		if (HAL_GPIO_ReadPin(RIGHT_BUTTON_GPIO_Port, RIGHT_BUTTON_Pin) == GPIO_PIN_SET) {
			// Button is held down - move the character
			mario.x_distance_between_frame += 3;
			move_tick = now;
	   }
		else if (HAL_GPIO_ReadPin(LEFT_BUTTON_GPIO_Port, LEFT_BUTTON_Pin) == GPIO_PIN_SET) {
			// Button is held down - move the character
			mario.x_distance_between_frame -= 3;
			move_tick = now;
		}
	   if (HAL_GPIO_ReadPin(JUMP_BUTTON_GPIO_Port, JUMP_BUTTON_Pin) == GPIO_PIN_SET) {
			// Button is held down - move the character
		   is_jumping = true;
		   jump_timer = HAL_GetTick();
		   mario.y_velocity = (mario.y_velocity) ? mario.y_velocity : -350;
		   move_tick = now;
		}
		read_pin_tick = now;
	}

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 12500;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 255;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|SD_Power_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RST_Pin|SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CS_Pin|DC_Pin|LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PA0 RST_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_Pin DC_Pin LED_Pin */
  GPIO_InitStruct.Pin = CS_Pin|DC_Pin|LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : JUMP_BUTTON_Pin RIGHT_BUTTON_Pin */
  GPIO_InitStruct.Pin = JUMP_BUTTON_Pin|RIGHT_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LEFT_BUTTON_Pin */
  GPIO_InitStruct.Pin = LEFT_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(LEFT_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_Power_Pin */
  GPIO_InitStruct.Pin = SD_Power_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SD_Power_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
//  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
