#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include "../libs/tensorflow/lite/c/c_api.h"

// FROM OTHER FILES ----
typedef struct Image {
	int width, height, channels;
	uint8_t *data;
} Image;

void alloc_image(Image *img) {
	img->data = (uint8_t*)malloc(img->width * img->height * img->channels);
}

void free_image(Image img) {
	free(img.data);
}

void read_image_raw_file(Image *img, const char *file) {
	FILE *f = fopen(file, "r");

	fread(&img->width, 4, 1, f);
	fread(&img->height, 4, 1, f);

	printf("%dx%d\n", img->width, img->height);

	img->channels = 3;

	alloc_image(img);

	fread(img->data, 1, img->width * img->height * 3, f);

	fclose(f);
}

void write_image_raw_file(Image img, const char *file) {
	FILE *f = fopen(file, "w");

	fwrite(&img.width, 4, 1, f);
	fwrite(&img.height, 4, 1, f);

	fwrite(&img.data, 1, img.width * img.height * 3, f);

	fclose(f);
}

void resize_image(Image src, Image *dest, int new_width, int new_height) {
	//printf("Resizing image from %dx%d to %dx%d\n", src.width, src.height, new_width, new_height);
	
	dest->width = new_width;
	dest->height = new_height;
	dest->channels = src.channels;
	alloc_image(dest);

	for(int i = 0; i < new_height; i++) {
		for(int j = 0; j < new_width; j++) {
			for(int k = 0; k < src.channels; k++) {
				int dest_idx = dest->channels * (i * new_width + j) + k;
				int src_row = i * src.height / new_height;
				int src_col = j * src.width / new_width;
				int src_idx = src.channels * (src_row * src.width + src_col) + k;

				dest->data[dest_idx] = src.data[src_idx];
			}
		}
	}
}

long long millis() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}
// ------------------

#define MODEL_PATH "../model.tflite"
#define TEST_IMAGE_BINARY_PATH "../test.bin"

#define NUM_DETECTIONS 10
#define WIDTH 320
#define HEIGHT 320
#define CHANNELS 3

typedef struct Victim {
	float x, y, d;
	int dead;
} Victim;

static TfLiteModel *model;
static TfLiteInterpreterOptions *options;
static TfLiteInterpreter *interpreter;

void victims_init() {
	model = TfLiteModelCreateFromFile(MODEL_PATH);
	options = TfLiteInterpreterOptionsCreate();
	TfLiteInterpreterOptionsSetNumThreads(options, 1);

	interpreter = TfLiteInterpreterCreate(model, options);

	if(TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
		printf("Error allocating tensors\n");
		return 1;
	}
}

void victims_destroy() {
	TfLiteInterpreterDelete(interpreter);
	TfLiteInterpreterOptionsDelete(options);
	TfLiteModelDelete(model);	
}

#define DETECTION_THRESHOLD 0.5f
#define DEAD_THRESHOLD 0.5f

int victims_detect(Image in, Victim *victims) {
	TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);	

	// Check dimensions
	if(in.channels != CHANNELS) {
		printf("Victim NN: Channel mismatch\n");
		return 0;
	}

	Image img;
	if(in.width != WIDTH || in.height != HEIGHT)	resize_image(in, &img, WIDTH, HEIGHT);
	else	img = in;

	// Normalize image and convert to float
	float input_image_norm[WIDTH * HEIGHT * CHANNELS];

	for(int i = 0; i < HEIGHT; i++) {
		for(int j = 0; j < WIDTH; j++) {
			for(int k = 0; k < CHANNELS; k++) {
				int idx = CHANNELS * (i * WIDTH + j) + k;
				input_image_norm[idx] = (float)img.data[idx] / 255.0f;
			}
		}
	}

	if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, WIDTH * HEIGHT * CHANNELS * sizeof(float)) != kTfLiteOk) {
		printf("Victim NN: Error copying input data\n");
		return 0;
	}

	TfLiteInterpreterInvoke(interpreter);

	float confidence[NUM_DETECTIONS];
	const TfLiteTensor *output_tensor0 = TfLiteInterpreterGetOutputTensor(interpreter, 0);
	TfLiteTensorCopyToBuffer(output_tensor0, confidence, NUM_DETECTIONS * sizeof(float));

	float boxes[NUM_DETECTIONS * 4];
	const TfLiteTensor *output_tensor1 = TfLiteInterpreterGetOutputTensor(interpreter, 1);
	TfLiteTensorCopyToBuffer(output_tensor1, boxes, NUM_DETECTIONS * 4 * sizeof(float));

	float classes[NUM_DETECTIONS];
	const TfLiteTensor *output_tensor2 = TfLiteInterpreterGetOutputTensor(interpreter, 3);
	TfLiteTensorCopyToBuffer(output_tensor2, classes, NUM_DETECTIONS * sizeof(float));

	int num_victims = 0;
	for(int i = 0; i < NUM_DETECTIONS; i++) {
		if(confidence[i] > DETECTION_THRESHOLD) {
			float ymin = boxes[i * 4 + 0];
			float xmin = boxes[i * 4 + 1];
			float ymax = boxes[i * 4 + 2];
			float xmax = boxes[i * 4 + 3];
		
			Victim v;
			v.x = (xmin + xmax) / 2.0f;
			v.y = (ymin + ymax) / 2.0f;
			v.d = (xmax - xmin + ymax - ymin) / 2.0f;
			v.dead = classes[i] > DEAD_THRESHOLD;

			victims[num_victims] = v;
			num_victims++;
		}
	}
	return num_victims;
}

#define NUM_IMAGES 4
const char* test_images[NUM_IMAGES] = {
	"../test_images/1.bin",
	"../test_images/2.bin",
	"../test_images/3.bin",
	"../test_images/4.bin"
};

int main() {
	victims_init();

	Image current_image;

	for(int i = 0; i < NUM_IMAGES; i++) {
		if(i > 0) free_image(current_image);

		read_image_raw_file(&current_image, test_images[i]);

		Victim victims[NUM_DETECTIONS];
		unsigned long start_time = millis();
		int num_victims = victims_detect(current_image, victims);
		unsigned long total_time = millis() - start_time;
		printf("Took: %lu\n", total_time);

		printf("Victims: %d\n", num_victims);
		for(int i = 0; i < num_victims; i++) {
			if(victims[i].dead) 	printf(" dead:   ");
			else 			printf(" living: ");
			printf("(%.3f, %.3f), %.3f\n", victims[i].x, victims[i].y, victims[i].d);
		}
	}

	victims_destroy();
	free_image(current_image);

	return 0;
}
