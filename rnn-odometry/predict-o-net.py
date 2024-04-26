import joblib
import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf
from keras.models import load_model
from sklearn.metrics import mean_absolute_error, r2_score
from sklearn.preprocessing import MinMaxScaler

# Load data
test_data = np.load('datasets/input_data_train.npy')
test_targets = np.load('datasets/input_targets_train.npy')

# Load model
model = load_model('models/model-20240425-204712')


# Load scaler
# scaler = joblib.load("scalers/scaler-20240425-095029")
# test_datascaler.fit()

# Plot and inspect loaded data
# num_channels = test_data.shape[1]
# plt.figure(figsize=(10, 5))
# # Plot each channel
# for i in range(num_channels):
#     plt.plot(test_data[:20000, i], label=f'Channel {i+1}')
# # Add a legend
# plt.legend()
# plt.show()

system_sample_rate = 115
sequence_length = system_sample_rate * 2    # Look back 3 seconds
sequence_stride = 1                         # Shift one sequence_length at a time (rolling window)
sampling_rate = 1                           # Used for downsampling
batch_size = 250
#NOTE: Using return_sequences=True. sequence_length should be = axis 0 of array (total sequence length)
# Create test dataset
test_dataset = tf.keras.utils.timeseries_dataset_from_array(
    data=test_data,
    targets=None,
    sequence_length=sequence_length,
    sequence_stride=sequence_stride,
    sampling_rate=sampling_rate,
    batch_size=batch_size
)

# # Predict
predictions = model.predict(test_dataset)

# Inspect model
# layer = model.get_layer('lstm_1')
# layer_activations = layer.output[:, 0, :]
# print(f"shape of layer activations: {layer_activations.shape}")
# # print(f'Sample activations from the layer: {layer_activations[0][:5]}')
# print(f"Model weights: {model.get_weights()}")

# Check shapes
print('test set shape:', test_data.shape)
print('prediction shape:', predictions.shape)
print('prediction shape[0]:', predictions.shape[0])
print('prediction shape[1]:', predictions.shape[1])
print('target set shape:', test_targets.shape)

# Evaluate
test_targets = test_targets[:predictions.shape[0]]
mae = mean_absolute_error(test_targets, predictions)
r2 = r2_score(test_targets, predictions)
print(f"Mean Absolute Error (MAE): {mae:.2f}")  # Print MAE
print(f"R-squared (R2) Score: {r2:.2f}")

# # Plot and inspect
plt.figure(figsize=(10, 6))
plt.plot(predictions, 'r', label='Predictions')
plt.plot(test_targets, 'b', label='Targets')
plt.legend()
plt.show()
