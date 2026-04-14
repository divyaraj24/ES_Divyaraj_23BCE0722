import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler


FEATURES = [
    "magnitude",
    "jerk",
    "variance",
    "peak_freq",
    "zero_cross",
    "duration",
]


def main():
    data = pd.read_csv("synthetic_training_data.csv")

    X = data[FEATURES].values
    y = tf.keras.utils.to_categorical(data["class_id"].values, num_classes=4)

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.1, random_state=42, stratify=data["class_id"].values
    )

    scaler = StandardScaler()
    X_train = scaler.fit_transform(X_train)
    X_test = scaler.transform(X_test)

    model = tf.keras.Sequential(
        [
            tf.keras.layers.Dense(10, activation="relu", input_shape=(6,)),
            tf.keras.layers.Dense(4, activation="softmax"),
        ]
    )

    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.01),
        loss="categorical_crossentropy",
        metrics=["accuracy"],
    )

    model.fit(
        X_train,
        y_train,
        epochs=200,
        batch_size=32,
        validation_split=0.1,
        callbacks=[tf.keras.callbacks.EarlyStopping(patience=15, restore_best_weights=True)],
        verbose=1,
    )

    loss, accuracy = model.evaluate(X_test, y_test, verbose=0)
    print(f"Test accuracy: {accuracy:.4f}")


if __name__ == "__main__":
    main()
