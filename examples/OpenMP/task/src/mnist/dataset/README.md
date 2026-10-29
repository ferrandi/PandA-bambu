# Optional MNIST CSV archives

These archives are used only by the manual performance suite. Normal regressions
use deterministic synthetic inputs. Their original download location and the
provenance of this CSV conversion were not recorded when they were added; do not
infer a license for the archives from the surrounding source headers.

Verify and extract from this directory:

```sh
sha256sum -c SHA256SUMS
unzip mnist_train.zip
unzip mnist_test.zip
```

Set `BAMBU_MNIST_TRAIN` and `BAMBU_MNIST_TEST` to the extracted CSV paths.
`SHA256SUMS` records the committed archive contents. A future external download
must preserve these checksums or explicitly document a replacement dataset.
