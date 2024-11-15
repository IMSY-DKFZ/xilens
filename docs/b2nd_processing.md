# Processing B2ND files
The default format of the files recorded with `XiLens` is the [n-dimensional arrays](https://www.blosc.org/python-blosc2-2.x/reference/ndarray_api.html)
developed by the `BLOSC2` team. These arrays provide a lot of useful features such as being able to store all images in a
single file in a lossless compression fashion.

## Visualize data
`XiLens` provides a simple viewer that can open the `.b2nd` files to visualize its contents. This can be done from the
`viewer` tab in `XiLens`. Alternatively, you can create your own viewer using `blosc2` to read the file contents.
You can see the next section for more information on how to open the files.

## Process data
We strongly recommend using `Python`'s library `blosc2` to open the `.b2nd` files. To do so, you first need to install
it:

```bash
conda create -n xilens python=3.10
conda activate xilens
pip install blosc2
```

Now you can open the contents of each file as follows:

```python
import blosc2

x = blosc2.open("path-to-file.b2nd", "r")
first_image = x[0, ...]

# print metadata
meta = x.schunk.vlmeta
print(meta.keys())
```

You should notice that opening the file does not load the data, it merely prepares everything to read it.
The data is only loaded when using slicing through the opened file as for the `first_image` example above.
The metadata of the loaded file behaves as a dictionary, so you can use it as such.
The metadata of these files contains useful information such as `time stamps`, camera `temperature`, etc.
