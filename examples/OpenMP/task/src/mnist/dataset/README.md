# Optional MNIST CSV archives

The image/label rows in both committed archives were compared with the public
[phoebetronic/mnist mirror](https://github.com/phoebetronic/mnist/tree/c3579c6434dc6ae12d6b65f2444a53918a0ab819),
commit `c3579c6434dc6ae12d6b65f2444a53918a0ab819`. All 60,000 training rows and
10,000 test rows match after normalizing line endings and dropping our header.
Our CSVs add the `label,1x1,...,28x28` header and use CRLF line endings.
The original download URL of these particular ZIP containers was not recorded;
the content comparison identifies an independently reproducible source.

The mirror attributes the CSV conversion to Joseph Redmon and publishes an
[MIT license](https://github.com/phoebetronic/mnist/blob/c3579c6434dc6ae12d6b65f2444a53918a0ab819/LICENSE),
reproduced in `LICENSE.phoebetronic`. It points to the original MNIST data by
Yann LeCun, Corinna Cortes and Christopher J. C. Burges. The upstream benchmark's
README also links to [MNIST in CSV](https://www.kaggle.com/datasets/oddrationale/mnist-in-csv),
whose dataset card declares CC0. Source-code Apache notices are not applied to
the dataset.

Normal regressions use deterministic synthetic inputs; these files are needed
only for optional real-data performance runs.

Verify and extract from this directory:

```sh
sha256sum -c SHA256SUMS
unzip mnist_train.zip
unzip mnist_test.zip
```

Alternatively, recreate identical CSV contents from the pinned GitHub mirror:

```sh
python3 fetch_mnist.py --output-dir /path/to/mnist-csv
```

Set `BAMBU_MNIST_TRAIN` and `BAMBU_MNIST_TEST` to the CSV paths. `SHA256SUMS`
records the committed ZIP containers; the download script checks both upstream
archive hashes and the reconstructed CSV hashes before installing either CSV.
