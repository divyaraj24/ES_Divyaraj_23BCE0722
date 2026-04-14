import csv
import random


CLASSES = {
    0: "MOVING",
    1: "IMPACT",
    2: "ROUGH",
    3: "STATIC",
}


def generate_sample(class_id):
    if class_id == 0:  # MOVING
        magnitude = 1.0 + random.gauss(0, 0.15)
        jerk = random.uniform(0.1, 0.4)
        variance = random.uniform(0.05, 0.15)
        peak_freq = random.uniform(5, 12)
        zero_cross = random.randint(8, 16)
        duration = 100
    elif class_id == 1:  # IMPACT
        magnitude = 2.5 + random.gauss(0, 0.5)
        jerk = random.uniform(1.5, 4.0)
        variance = random.uniform(0.3, 0.8)
        peak_freq = random.uniform(15, 25)
        zero_cross = random.randint(15, 25)
        duration = random.uniform(20, 50)
    elif class_id == 2:  # ROUGH
        magnitude = 1.5 + random.gauss(0, 0.3)
        jerk = random.uniform(0.3, 0.8)
        variance = random.uniform(0.2, 0.4)
        peak_freq = random.uniform(8, 15)
        zero_cross = random.randint(12, 20)
        duration = 100
    else:  # STATIC
        magnitude = 1.0 + random.gauss(0, 0.02)
        jerk = random.uniform(0, 0.05)
        variance = random.uniform(0, 0.02)
        peak_freq = random.uniform(0, 2)
        zero_cross = random.randint(0, 3)
        duration = 100

    return [
        round(magnitude, 4),
        round(jerk, 4),
        round(variance, 4),
        round(peak_freq, 4),
        zero_cross,
        round(duration, 2),
        class_id,
        CLASSES[class_id],
    ]


def main():
    random.seed(42)
    rows = []
    samples_per_class = 1000

    for class_id in range(4):
        for _ in range(samples_per_class):
            rows.append(generate_sample(class_id))

    random.shuffle(rows)

    with open("synthetic_training_data.csv", "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow([
            "magnitude",
            "jerk",
            "variance",
            "peak_freq",
            "zero_cross",
            "duration",
            "class_id",
            "class_label",
        ])
        writer.writerows(rows)

    print(f"Generated {len(rows)} rows in synthetic_training_data.csv")


if __name__ == "__main__":
    main()
