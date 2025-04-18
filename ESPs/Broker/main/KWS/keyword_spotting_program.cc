/*
 *  keyword_spotting_program.cc
 *
 *  Created on: july 18 , 2024
 *  Author: mohammedhamdy32
 */


/* TensorFlow lite Micro C++ libraries */
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h" /* Provides the operations used by the interpreter to run the model.*/
#include "tensorflow/lite/schema/schema_generated.h"         /* Contains the schema for the TensorFlow Lite FlatBuffer model file format. */
#include "tensorflow/lite/micro/micro_interpreter.h"         
#include "tensorflow/lite/micro/system_setup.h"
#include "other/recognize_commands.h"
#include "other/audio_provider.h"
#include "other/command_responder.h"
#include "other/micro_model_settings.h"
#include "other/yes_micro_features_data.h"
#include <esp_log.h>
#include "esp_heap_caps.h"


/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "other/micro_model_settings.h"
#include "driver/i2s.h"
#include "driver/gpio.h"

/* Other C libraries */
extern "C" {
#include "Network/Network_inteface.h"
#include "other/feature_provider.h" /* This library used to extract audio features from the input data, like spectogram */
#include "keyword_spotting_interface.h"
#include "keyword_spotting_config.h"
#include "keyword_spotting_model.h"
}

#define USED_PSRAM 1

/* TAG used for serial message */
static const char TAG[] = "KWS app";

/* keyword spotting status (created or deleted) */
static uint8_t g_keyword_spotting_task_status = pdFALSE;

/* Keyword spotting task handler */
static TaskHandle_t g_keyword_spotting_task_handler;


/* Reset the number of slices */
extern bool g_reset_slice_needed;


extern uint8_t is_new_data;
extern uint8_t g_recvData[30];
extern uint8_t g_recvTopic[30];
extern uint8_t g_recvDataLen;
extern uint8_t g_recvTopicLen;


/* Static function prototype */
static void keyword_spotting_Init(void);
static void keyword_spotting_loop(void);
static void keyword_spotting_app_task(void *pvParameter);


/*** Declare variable ***/
/* Globals, used for compatibility with Arduino-style sketches. */
namespace {
const tflite::Model* g_myModel = nullptr; /* Contains our model */
tflite::MicroInterpreter* g_interpreter = nullptr; /* An interpreter used to */
tflite::ErrorReporter* g_error_reporter = nullptr; /* Used to handle error reporting */  
TfLiteTensor* g_input = nullptr;  /* My input  */
TfLiteTensor* g_output = nullptr; /* My output */

int32_t g_previous_time = 0; 
int8_t* g_model_input_buffer = nullptr; /* Input buffer */


/* In tensorFlow micro, they avoid any dynamic allocation, to avoid fragmentation, so we declare an array, 
   which the interpreter uses during runtime, so use try and error to know the best size of this array */
constexpr int g_kTensorArenaSize = 50*1024; /* constexpr : means that the variable ia a constant expression,
 which means its value is determined at compile time rather than at runtime, like #define, so no dynamic allocation occur */

/* g_tensor_arena is an array that the interpreter will use during run time */
/* If we use a externa PSRAM, so we will allocate the arena in it, not in internal ram */
#if ( USED_PSRAM == 1 )
    uint8_t *g_tensor_arena = (uint8_t *) heap_caps_malloc( g_kTensorArenaSize , MALLOC_CAP_SPIRAM ) ; 
#else
    uint8_t g_tensor_arena [g_kTensorArenaSize]; 
#endif   

FeatureProvider* g_feature_provider = nullptr; /* Feature provide (spectogram) */
RecognizeCommands* g_recognizer = nullptr; /* Recongnizer to store output of our network */
#if ( USED_PSRAM == 1 )
  int8_t *g_feature_buffer = (int8_t *) heap_caps_malloc( g_kFeatureElementCount , MALLOC_CAP_SPIRAM ) ; 
#else  
  int8_t g_feature_buffer[g_kFeatureElementCount]; /* Contains the actual spectogram */
#endif

}/* namespace */


static void keyword_spotting_Init(void)
{
    /*** Load model ***/
    /* Map the model into a usable data structure. This doesn't involve any
       copying or parsing, it's a very lightweight operation. */
    g_myModel = tflite::GetModel(g_model);
    /* Check model version */
    if( g_myModel->version() != TFLITE_SCHEMA_VERSION ) 
    {
      MicroPrintf("Model provided is schema version %d not equal to supported "
      "version %d.", g_myModel->version(), TFLITE_SCHEMA_VERSION);
      return;
    }

    /*** Resolve operator ***/
    /* Put only the operation implementations we need to save reduce memory usage, like conv2D, conv3D or sigmoid*/
    /* We can use netron web page to see the operators in the model */
    static tflite::MicroMutableOpResolver<12> resolver; /* I will use 4 operator */
    if (resolver.AddFullyConnected()/*Dense*/ != kTfLiteOk)  
    { 
        return;
    }
    // if( resolver.AddDepthwiseConv2D()/*Conv2D*/ != kTfLiteOk )
    // {
    //   return;
    // } 
    if( resolver.AddSoftmax()/*Softmax*/ != kTfLiteOk )
    {
      return;
    }
    if( resolver.AddReshape() != kTfLiteOk )
    {
      return;
    }
    if( resolver.AddConv2D() != kTfLiteOk )
    {
      return;
    }
    // if( resolver.AddRelu() != kTfLiteOk )
    // {
    //   return;
    // }
    if( resolver.AddMaxPool2D() != kTfLiteOk )
    {
      return;
    }
    // if( resolver.AddMul() != kTfLiteOk )
    // {
    //   return;
    // }
    // if( resolver.AddAdd() != kTfLiteOk )
    // {
    //   return;
    // }
    // if( resolver.AddLogistic() != kTfLiteOk )
    // {
    //   return;
    // }
    // if( resolver.AddQuantize() != kTfLiteOk )
    // {
    //   return;
    // }
    // if( resolver.AddDequantize() != kTfLiteOk )
    // {
    //   return;
    // }



  

    /*** Initalize interpreter ***/
    /* Initalize the interpreter to run the model with. */
    /* It takes the Mymodel, resolver, tensor arena and arean size */
    static tflite::MicroInterpreter static_interpreter( g_myModel, resolver, g_tensor_arena, g_kTensorArenaSize );
    g_interpreter = &static_interpreter;


    /*** Allocate Arena in interpreter ***/
    TfLiteStatus allocate_status = g_interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) 
    {
      MicroPrintf("AllocateTensors() failed");
      return;
    }


    /*** Define Model inputs ***/
    /* The input size is defines in the model array */
    /* Gives the interpreter where the input buffer is actually stored */
    g_input = g_interpreter->input(0); /* Inialize the input */
    if ( (g_input->dims->size != 4 ) ||  /* The input is 4D, the first dimention is a wrapper, and the second is our spectogram */
    (g_input->dims->data[0] != 1) || /* Check the wrapper size */
    ( g_input->dims->data[1]/*Spectogram arr size*/ != 49  ) || 
    (g_input->type != kTfLiteInt8 ) ) /* Make sure that the datatype is int8 */
    {
      printf("%d  %d, spectogram shape=%d\n" , g_input->dims->size , g_input->dims->data[0] , g_input->dims->data[1]  );
      MicroPrintf("Bad input tensor parameters in model : spectogram=%d %d %d %d, type=%d , array size=%d" , g_input->dims->data[0] ,g_input->dims->data[1] , g_input->dims->data[2] , g_input->dims->data[3] , g_input->type , g_input->dims->size  );
      // return;
    }
    printf("%d  %d, spectogram shape=%d\n" , g_input->dims->size , g_input->dims->data[0] , g_input->dims->data[1]  );
    g_model_input_buffer = tflite::GetTensorData<int8_t>(g_input);

    /*** Setup main loop ***/
    // Prepare to access the audio spectrograms from a microphone or other source
    // that will provide the inputs to the neural network.
    // NOLINTNEXTLINE(runtime-global-variables)

    /* This function access the audio, and convert it to spectogram  */
    static FeatureProvider static_feature_provider( g_kFeatureElementCount , g_feature_buffer);
    g_feature_provider = &static_feature_provider;
    
    /* Recognize the command */
    static RecognizeCommands static_recognizer(g_error_reporter);
    g_recognizer = &static_recognizer;
    g_previous_time = 0;

}

uint8_t g_flag = 0;
static void keyword_spotting_loop(void)
{
    if(g_reset_slice_needed)
    {
      g_previous_time = 0;
    }
    const int32_t current_time = LatestAudioTimestamp(); /*LatestAudioTimestamp is a function that retrieves the current timestamp of the latest audio data available.*/
    int how_many_new_slices = 0; /* How many slices (columbs) we need */
    /* This line fills the feature buffer with audio data between the given timestamp, and convert it to spectogram */
    /* g_previous_time is the last time you called the PopulateFeatureData function
       current_time is the time now 
       we pass those parameters to know how much time is elapsed since this function is called, to know how much data will be added to generate the spectogram */
#if USE_FFT
    TfLiteStatus feature_status = g_feature_provider->PopulateFeatureData( g_error_reporter , g_previous_time,current_time,&how_many_new_slices);
#else
    TfLiteStatus feature_status = g_feature_provider->PopulateFeatureData( g_previous_time,current_time,&how_many_new_slices);
#endif

    // printf("%d\n" , how_many_new_slices );

    if(feature_status != kTfLiteOk) 
    {
      MicroPrintf( "Feature generation failed");
      return;
    }  
    g_previous_time = current_time; /* Put current time into previous time which is the last time we call the PopulateFeatureData function */

    /* If no new audio samples have been received since last time, don't bother running the network model. */
    if (how_many_new_slices == 0) 
    {
      return;
    }

    /* Copy feature buffer(spectogram) to input tensor of the model */
    for (int i = 0; i < g_kFeatureElementCount; i++)
    {
      g_model_input_buffer[i]/*Input to the model*/ = g_feature_buffer[i]/*Spectogram*/;
      // printf( "%d," , g_model_input_buffer[i] );
    }
    // printf("\n\n\n");

    /*** Inference stage ***/
    /* Call the interpreter to run the model.*/
    TfLiteStatus invoke_status = g_interpreter->Invoke();
    if (invoke_status != kTfLiteOk) 
    {
      MicroPrintf( "Invoke failed");
      return;
    }

    /*** Post-processing stage ***/
#if(0)
    /* How does this method work? */
    /* For every new window
     * 1) Store new infrence 
     * 2) Calculate new score for all words
     * 3) Output new average score
     */
    TfLiteTensor* output = g_interpreter->output(0); /* A pointer to the output out network */
    const char* found_command = nullptr; /* True if a command is found, False if not */
    uint8_t score = 0;
    bool is_new_command = false;
    /* This function make saves the last inferene and take the average between them to make prediction */
    TfLiteStatus process_status = g_recognizer->ProcessLatestResults(output, current_time, &found_command, &score, &is_new_command);

    if (process_status != kTfLiteOk) 
    {
      MicroPrintf("RecognizeCommands::ProcessLatestResults() failed");
      return;
    }

    /* This function is the action that will be taken with the predicted command */
    RespondToCommand(current_time, found_command, score, is_new_command); 

#else
    TfLiteTensor* output = g_interpreter->output(0);
    float output_scale = output->params.scale; /* 0.0039062 */
    int output_zero_point = output->params.zero_point;  /* -128 */

    /* Find the max keyword from kCategoryCount words */
    int max_idx = 0;
    float max_result = 0.0;
    for (int i = 0; i < kCategoryCount; i++) 
    {
      /* The output is from -127 to 128, so we will quantive it first */
      float current_result = (tflite::GetTensorData<int8_t>(output)[i] - output_zero_point) * output_scale;
      /* Find max word percent */
      if (current_result > max_result) 
      {
        max_result = current_result; // update max result
        max_idx = i; // update category
      }
    }

    if ( max_result > 0.9f ) 
    {

      if( kCategoryLabels[max_idx][0] == 'O' && kCategoryLabels[max_idx][1] == 'N' )
      {
        MicroPrintf("Detected %7s, score: %.2f", kCategoryLabels[max_idx] , static_cast<double>(max_result));
        is_new_data = 1;
        memcpy( (char *)g_recvData  , "2{\"action\": \"servo\", \"value\": 180.0, \"timestamp\": \"2025-04-16T14:34:30.729291\"}\n"  , 81 );
        g_recvDataLen = 81;
      }
      else if( kCategoryLabels[max_idx][0] == 'O' && kCategoryLabels[max_idx][1] == 'F' )
      {
        MicroPrintf("Detected %7s, score: %.2f", kCategoryLabels[max_idx] , static_cast<double>(max_result));
        is_new_data = 1;
        memcpy( (char *)g_recvData  , "2{\"action\": \"servo\", \"value\": 0.000, \"timestamp\": \"2025-04-16T14:34:30.729291\"}\n"  , 81 );
        g_recvDataLen = 81;      
      }

    }

#endif  
    /* To reset watchdog */
    vTaskDelay( 5000/portMAX_DELAY );

}

/* The Task function for freeRTOS */
static void keyword_spotting_app_task(void *pvParameter)
{
  /* Initalize keyword spotting */
  keyword_spotting_Init();
  
  /* Keyword spotting loop*/
	ESP_LOGI( TAG , "Entring infinity loop" );
  for(;;)
  {
    keyword_spotting_loop();
  }

}


void keyword_spotting_app_start(void)
{

  if( g_keyword_spotting_task_status == pdFALSE )
  {
      ESP_LOGI( TAG , "Starting keyword spotting Application" );

      // void *psramTaskStack = heap_caps_malloc( KEYWORD_SPOTTING_APP_TASK_STACK_SIZE , MALLOC_CAP_SPIRAM );
      // if (psramTaskStack == NULL) {
      //     printf("Failed to allocate KWS task stack in PSRAM\n");
      //     configASSERT(1);
      //     return;
      // }
	    // BaseType_t TaskStatus = xTaskCreatePinnedToCore( &keyword_spotting_app_task , "keyword task" , KEYWORD_SPOTTING_APP_TASK_STACK_SIZE , NULL , KEYWORD_SPOTTING_APP_TASK_PRIORITY , &g_keyword_spotting_task_handler , KEYWORD_SPOTTING_APP_TASK_CORE_ID );
	    // BaseType_t TaskStatus = xTaskCreateStaticPinnedToCore( &keyword_spotting_app_task , "keyword task" , KEYWORD_SPOTTING_APP_TASK_STACK_SIZE , NULL , KEYWORD_SPOTTING_APP_TASK_PRIORITY , (StackType_t *)psramTaskStack , NULL , KEYWORD_SPOTTING_APP_TASK_CORE_ID );

      /* Start keyword spotting task in FreeRTOS */
      BaseType_t TaskStatus = xTaskCreatePinnedToCore( &keyword_spotting_app_task , "keyword task" , KEYWORD_SPOTTING_APP_TASK_STACK_SIZE , NULL , KEYWORD_SPOTTING_APP_TASK_PRIORITY , &g_keyword_spotting_task_handler , KEYWORD_SPOTTING_APP_TASK_CORE_ID );
      configASSERT(TaskStatus == pdPASS); /* Is a MACRO, If the condition is false, It will enter an infinity loop */
      g_keyword_spotting_task_status = pdTRUE;

  }

}



void keyword_spotting_app_suspend(void)
{
  
  if( g_keyword_spotting_task_status == pdTRUE )
  {
      /* Delete keyword spotting task in FreeRTOS */
      ESP_LOGI( TAG , "Suspending keyword spotting Application" );

      vTaskSuspend( g_keyword_spotting_task_handler );
      // vTaskSuspend( g_capture_audio_task_handler );
      
      uint8_t l_32bit_audio_buffer[10];
      size_t data_size;
      ESP_ERROR_CHECK( i2s_read( I2S_NUM , (void *)(l_32bit_audio_buffer) , 1 , &data_size , pdMS_TO_TICKS(100) /*Timeout*/ ) ); 

      g_keyword_spotting_task_status = pdFALSE;
  }

}

void keyword_spotting_app_relese(void)
{
  
  if( g_keyword_spotting_task_status == pdFALSE )
  {
      /* Delete keyword spotting task in FreeRTOS */
      ESP_LOGI( TAG , "Relsese keyword spotting Application" );

      /* Close I2S driver */
      // i2s_driver_install( I2S_NUM , &i2s_config, 0 , NULL );

      vTaskResume( g_keyword_spotting_task_handler );
      g_reset_slice_needed = true;
      // vTaskResume( g_capture_audio_task_handler );

      g_keyword_spotting_task_status = pdTRUE;
  }

}







