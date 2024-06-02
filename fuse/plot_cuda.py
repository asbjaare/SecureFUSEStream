import matplotlib.pyplot as plt
import numpy as np


def read_timing_data(filename):
    with open(filename, "r") as file:
        data = file.read()

    blocks = data.strip().split("\n\n")
    timing_data = []
    for block in blocks:
        lines = block.split("\n")
        times = [float(line.split(": ")[1]) for line in lines]
        timing_data.append(times)

    return timing_data


def plot_timing_data(timing_data):
    num_measurements = [2**i for i in range(len(timing_data))]
    means = [np.mean(times) for times in timing_data]
    std_devs = [np.std(times) for times in timing_data]

    plt.figure(figsize=(10, 6))
    plt.errorbar(num_measurements, means, yerr=std_devs, fmt="o", capsize=5)
    plt.plot(
        num_measurements,
        means,
        marker="o",
        linestyle="-",
        color="black",
        label="Trendline",
    )

    plt.xscale("log", base=2)
    plt.xticks(num_measurements, [f"{2**i}x" for i in range(len(timing_data))])
    plt.xlabel("Image size")
    plt.ylabel("Mean Time (ms)")
    plt.title("Time vs Image Size")
    plt.savefig("plot_cuda.svg", format="svg")
    plt.show()


# Read data from file
timing_data = read_timing_data("timings.data")

# Plot the data
plot_timing_data(timing_data)
