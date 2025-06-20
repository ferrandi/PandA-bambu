import matplotlib.pyplot as plt
import os


def load_sets_from_file(filename):
    sets = {}
    current_set = None
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                set_name = line[1:].strip()
                current_set = f"{os.path.basename(filename)}: {set_name}"
                sets[current_set] = ([], [])
            else:
                if current_set is None:
                    raise ValueError(f"Data found before header in {filename}")
                t, y = map(float, line.split())
                sets[current_set][0].append(t)
                sets[current_set][1].append(y)
    return sets


def main(filenames):
    all_sets = {}
    for file in filenames:
        all_sets.update(load_sets_from_file(file))

    plt.figure(figsize=(10, 4))
    for name, (tlist, ylist) in all_sets.items():
        plt.plot(tlist, ylist, label=name)

    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude")
    plt.title("Multi-File Signal Plot")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()


# If run from notebook, call main manually
if __name__ == "__main__":
    # fallback if you run from shell
    import sys

    if len(sys.argv) < 2:
        print("Usage: python plot_sets.py file1.txt [file2.txt ...]")
    else:
        main(sys.argv[1:])
