#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

#include "layers/dense.hpp"
#include "layers/relu.hpp"
#include "optimizers/sgd.hpp"
#include "loss/crossentropy.hpp"
#include "models/sequential.hpp"
#include "datasets/mnist.hpp"
#include "loggers/csv_logger.hpp"
#include "utils.hpp"
#include "configuration.hpp"

#include "cuda_runtime.h"

long long elapsed_time_ms;
extern void do_kernel_init();
extern "C" int main(int argc, char** argv) {

    do_kernel_init();

    // Always initialize seed to some random value
    srand(static_cast<unsigned>(time(0)));

    // Prepare events for measuring time on CUDA
    float elapsedTime = 0.0;
    //cudaEvent_t start, end;
    //cudaEventCreate(&start);
    //cudaEventCreate(&end);
    struct timeval start, end;

    // Print our current configuration for this training
    // Configuration::printCurrentConfiguration();
    // Configuration::printCUDAConfiguration();

    // Read both training and test dataset
    MNISTDataSet* trainDataset = new MNISTDataSet(TRAIN);
    MNISTDataSet* testDataset = new MNISTDataSet(TEST);

    fprintf(stderr, "data loading fin\n");

    // Prepare optimizer and loss function
    float learningRate = Configuration::learningRate;
    SGDOptimizer* optimizer = new SGDOptimizer(learningRate);
    CrossEntropyLoss* loss = new CrossEntropyLoss();

    // Prepare model
    SequentialModel* model = new SequentialModel(optimizer, loss);
    model->addLayer(new DenseLayer(28*28, 100));
    model->addLayer(new ReLuLayer(100));
    model->addLayer(new DenseLayer(100, 10));

    // Prepare logger that will help us gather timings from experiments
    //CSVLogger* logger = new CSVLogger(Configuration::logFileName);

    // Run some epochs
    int epochs = Configuration::numberOfEpochs;
    int batchSize = Configuration::batchSize;
    int numberOfTrainBatches = trainDataset->getSize() / batchSize;
    int numberOfTestBatches = testDataset->getSize() / batchSize;
    // fprintf(stderr, "NUmber of train batches:%d\n",numberOfTrainBatches);
   
    numberOfTrainBatches = numberOfTrainBatches / 2;
    fprintf(stderr, "NUmber of train batches:%d\n",numberOfTrainBatches);
    
    elapsed_time_ms = 0;

    for (int epoch = 0; epoch < epochs; epoch++) {
        float trainingLoss = 0.0, trainingAccuracy = 0.0;
        double trainingForwardTime = 0.0, trainingBackwardTime = 0.0;
        fprintf(stderr, "Epoch %d:\n", epoch);
        for (int batch = 0; batch < numberOfTrainBatches; batch++) {
            // Fetch batch from dataset
            Tensor2D* images = trainDataset->getBatchOfImages(batch, batchSize);
            Tensor2D* labels = trainDataset->getBatchOfLabels(batch, batchSize);

            // Forward pass
            //cudaEventRecord(start, 0);
            gettimeofday(&start, 0);
            Tensor2D* output = model->forward(images);
            //cudaEventRecord(end, 0);
            //cudaEventSynchronize(end);
            cudaDeviceSynchronize();

            // Save statistics
            trainingLoss += loss->getLoss(output, labels);
            trainingAccuracy += loss->getAccuracy(output, labels);
            gettimeofday(&end, 0);
            elapsedTime = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
            //cudaEventElapsedTime(&elapsedTime, start, end);
            trainingForwardTime += elapsedTime;

            // Backward pass
            //cudaEventRecord(start, 0);
            gettimeofday(&start, 0);
            model->backward(output, labels);
            //cudaEventRecord(end, 0);
            //cudaEventSynchronize(end);
            cudaDeviceSynchronize();

            // Save statistics
            gettimeofday(&end, 0);
            elapsedTime = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
            //cudaEventElapsedTime(&elapsedTime, start, end);
            trainingBackwardTime += elapsedTime;

            // Clean data for this batch
            delete images;
            delete labels;
        }
	
        // Calculate mean training metrics
        trainingLoss /= numberOfTrainBatches;
        trainingAccuracy /= numberOfTrainBatches;
        // printf("  - [Train] Loss=%.5f\n", trainingLoss);
        // printf("  - [Train] Accuracy=%.5f%%\n", trainingAccuracy);
        // printf("  - [Train] Total Forward Time=%.5fms\n", trainingForwardTime);
        // printf("  - [Train] Total Backward Time=%.5fms\n", trainingBackwardTime);
        fprintf(stderr, "  - [Train] Batch Forward Time=%.5fms\n", trainingForwardTime / numberOfTrainBatches);
        fprintf(stderr, "  - [Train] Batch Backward Time=%.5fms\n", trainingBackwardTime / numberOfTrainBatches);
        
        elapsed_time_ms += (trainingForwardTime + trainingBackwardTime) / numberOfTrainBatches;

        // Check model performance on test set
        float testLoss = 0.0, testAccuracy = 0.0;
        for (int batch = 0; batch < numberOfTestBatches; batch++) {
            // Fetch batch from dataset
            Tensor2D* images = testDataset->getBatchOfImages(batch, batchSize);
            Tensor2D* labels = testDataset->getBatchOfLabels(batch, batchSize);

            // Forward pass
            Tensor2D* output = model->forward(images);

            // Print error
            testLoss += loss->getLoss(output, labels);
            testAccuracy += loss->getAccuracy(output, labels);

            // Clean data for this batch
            delete images;
            delete labels;
        }

        // Calculate mean testing metrics
        testLoss /= numberOfTestBatches;
        testAccuracy /= numberOfTestBatches;
        // fprintf(stderr, "  - [Test] Loss=%.5f\n", testLoss);
        // fprintf(stderr, "  - [Test] Accuracy=%.5f%%\n", testAccuracy);
        // fprintf(stderr, "\n");

        // Save times to the logger
        	
        //logger->logEpoch(trainingLoss, trainingAccuracy,
        //                 testLoss, testAccuracy,
        //                 trainingForwardTime, trainingBackwardTime,
        //                 trainingForwardTime / numberOfTrainBatches,
        //                 trainingBackwardTime / numberOfTrainBatches);
	
        // Shuffle both datasets before next epoch!
        trainDataset->shuffle();
        testDataset->shuffle();
        	
    }

    elapsed_time_ms = elapsed_time_ms / epochs;

    //delete logger;
    return 0;
}
