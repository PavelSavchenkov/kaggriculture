# This Python 3 environment comes with many helpful analytics libraries installed
# It is defined by the kaggle/python Docker image: https://github.com/kaggle/docker-python
# For example, here's several helpful packages to load

import numpy as np # linear algebra
import pandas as pd # data processing, CSV file I/O (e.g. pd.read_csv)

# Input data files are available in the read-only "../input/" directory
# For example, running this (by clicking run or pressing Shift+Enter) will list all files under the input directory

import os
for dirname, _, filenames in os.walk('/kaggle/input'):
    for filename in filenames:
        print(os.path.join(dirname, filename))

# You can write up to 20GB to the current directory (/kaggle/working/) that gets preserved as output when you create a version using "Save & Run All" 
# You can also write temporary files to /kaggle/temp/, but they won't be saved outside of the current session

# Use the kagglehub client library to attach Kaggle resources like competitions, datasets, and models to your session
# Learn more about kagglehub: https://github.com/Kaggle/kagglehub/blob/main/README.md

import kagglehub
# kagglehub.dataset_download('<owner>/<dataset-slug>')

import math
from mpmath import mp

# 118 Periodic Table Elements
ELEMENTS = [
    "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne",
    "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca",
    "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn",
    "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr",
    "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn",
    "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
    "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb",
    "Lu", "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg",
    "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th",
    "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm",
    "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds",
    "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og"
]

def generate_pi_digits(precision: int = 10000) -> str:
    """Generates pi digits as a string excluding the decimal point."""
    mp.dps = precision
    return str(mp.pi).replace(".", "")

def text_to_pi_elements(text: str, pi_digits: str) -> list[dict]:
    """
    Encodes text into periodic table elements and finds matching pi positions.
    """
    encoded_sequence = []
    
    for char in text:
        # Convert character to ASCII code and map to valid atomic number (1-118)
        ascii_val = ord(char)
        atomic_num = (ascii_val % len(ELEMENTS)) + 1
        symbol = ELEMENTS[atomic_num - 1]
        
        # Search for atomic number string inside Pi digits
        target_str = str(atomic_num).zfill(2)
        pi_index = pi_digits.find(target_str)
        
        encoded_sequence.append({
            "character": char,
            "ascii": ascii_val,
            "atomic_number": atomic_num,
            "symbol": symbol,
            "pi_digits_match": target_str,
            "pi_position": pi_index if pi_index != -1 else "Not found in range"
        })
        
    return encoded_sequence

# Example Usage
pi_pool = generate_pi_digits(precision=5000)
input_text = "Science"

results = text_to_pi_elements(input_text, pi_pool)

print(f"{'Char':<6} | {'ASCII':<6} | {'Atomic #':<9} | {'Element':<8} | {'Pi Pos':<8}")
print("-" * 50)
for r in results:
    print(f"{r['character']:<6} | {r['ascii']:<6} | {r['atomic_number']:<9} | {r['symbol']:<8} | {r['pi_position']:<8}")


# Kaggle Packages

***

### Overview

Kaggle Packages are new functionality that lets you write Python Packages which can be imported and re-used elsewhere. We use the open-source `nbdev` library (more info on their [homepage](https://nbdev.fast.ai)) to let you define a Python Package within a Kaggle Notebook, and our `kagglehub` library enables you to import and re-use it elsewhere.

One core benefit of Kaggle Packages is that it simplifies the user experience of participating in Code Competitions which support them. Previously, in most Code Competitions your Notebook would have to read the test set file(s) from Kaggle-specific filepaths, run the inference loop yourself while keeping track of a Kaggle-specific `id` column, then carefully package your predictions and `id` values into a `submission.csv` file written to another Kaggle-specific filepath. With Kaggle Packages, you no longer have to worry about those task-orthogonal details, instead you just write inference code which implements the competition's ML task and we take care of the rest.

Furthermore, Package submissions should be easier to re-use. You can use `kagglehub` to import a Package and call its code with arbitrary inputs anywhere. See below for more detailed instructions.

The initial intention is to use Kaggle Packages within (some) Code Competitions -- for example we're launching alongside the [Drawing With LLMs competition](https://www.kaggle.com/competitions/drawing-with-llms) -- though you can use them outside of Competitions as well, and we hope to expand our support there.

***

### What is a Kaggle Package

A Kaggle Package is a Python package generated from a Kaggle Notebook. It's created using `nbdev`, which exports specific cells (marked with `#| export`) from your notebook into Python files. The resulting Package is located in the `package` subdirectory of your Notebook's Output.

A Kaggle Package has the following structure:

- `__init__.py`: This file marks the directory as a Python package and defines metadata like the Docker Image and GPUs used when your Package was created, and optional Dependency Manager configuration.
- `*.py`: Submodule files contain the code you exported from your Notebook using `nbdev`. The `#| default_exp` directive in your Notebook determines the main module name (e.g., `core.py`).
- `assets/` (optional): This subdirectory stores any asset files your package needs, such as model weights, configuration files, or data files. You can access these files using `kagglehub.get_package_asset_path()`.
- `kagglehub_requirements.yaml`: This file lists the Kaggle resources (Datasets, Models, Notebooks, Packages) that your package depends on, including specific versions for reproducibility reasons.

Example Package Structure:

```codeBlock
package/
├── __init__.py
├── core.py
├── kagglehub_requirements.yaml
└── assets/
    └── model.weights
```

***

### Creating a Package

To create a Kaggle Package, you'll write a Kaggle Notebook using `nbdev` conventions. Here's a breakdown of the process:

1. **Start with a Kaggle Notebook:** Create a new Kaggle Notebook or use an existing one.
2. **Use `nbdev` directives:**
   - Add `#| default_exp core` (or another module name) to a code cell. This is required and specifies the main module for your Package.
   - Mark the code cells you want to export with `#| export`. Only these cells will be included in your Package.
3. **Define your Package logic:** Write the code for your package, making sure to export the desired parts and not export undesired parts. This conditional export power is one main goal of the `nbdev` patterns, so you can define your exported Package and within the same Notebook also run other code which tests or analyzes your core functionality but which itself is \*not\* exported.
   - Make sure your exported code includes all `import` statements which its code requires.
   - Use `kagglehub` to refer to any Kaggle resources (Models, Datasets, Notebooks, other Packages). See below for more information.
   - For Code Competitions, we require a `class Model` with a `predict()` method meeting the competition's required input/output spec.
4. **Add Asset Files (Optional):** If your package needs external files, save them using `kagglehub.get_package_asset_path()` and have your Package code read the file using that same function.
5. **Configure Python Dependencies (Optional):** If you need Python Packages which are not available in our base environment, you can use our Dependency Manager to add them. See below for more information.
6. **Save Version:** When you Save your Notebook, Kaggle will run your notebook as usual, then generate and validate your Package which gets saved to your Notebook's output.

Your generated Package has some special logic that gets applied when it is `import`ed. If you used the Dependency Manager feature, your dependencies will be installed. We'll then `import` the submodule(s) with your code (such as `core` which comes from `#| default_exp core`) and we expose all public data members from those submodule(s) (those without a leading underscore) onto the top-level Python module; this means if you export `class Model` then you'll have `package.Model` available directly.

#### 1. Using `kagglehub` for Kaggle resource dependencies

If your Package needs to use other Kaggle resources (Datasets, Models, Notebooks, or other Packages), you must use `kagglehub` to access them. This ensures that your package remains portable and doesn't rely on Kaggle-specific filepaths like `/kaggle/input`. Your Notebook \*must\* have all datasources attached, either through the Notebook Editor sidebar, or by executing the `kagglehub` command, before you `Save Version`, since the Save execution is not allowed to attach new datasources, or even different versions of datasources.

Example (loading a Kaggle Model):

```codeBlock
#| export
import kagglehub
import keras

class Model:
  def __init__(self):
    model_path = kagglehub.model_download('user/model/framework/variation')
    # OR model_download('user/model/framework/variation/version')
    self.model = keras.saving.load_model(model_path)

  def predict(self, features):
    return self.model.predict(features)
```

You can use the `Copy kagglehub command` option in the Notebook Editor Input sidebar to get the correct command for a given resource.

Note that there are currently some limitations on accessing older versions of Dataset, Notebook, or Package datasources, see the Known Issues section below for more details.

#### 2. Using Dependency Manager to import Python dependencies

Kaggle Notebooks have many popular python packages pre-installed in their base Docker Image, but there's a lot of great packages not pre-installed which you may want to use. Kaggle Notebooks have a Dependency Manager tool (see [documentation](https://www.kaggle.com/discussions/product-announcements/532336)) which not only installs external python packages into your Notebook, but also saves their version so that your Notebook (or exported Package) will use that same version when re-used later. This is important for our goal of having reproducible artifacts, and also means that your Notebook (or Package) can use those dependencies in a competition scoring session where internet access is not allowed.

In the Notebook Editor menu select `Add-ons` -&gt; `Install Dependencies` and write your `pip install ...` commands. In your Interactive Notebook Editor session, you'll need to manually `Run` from the `Dependency Manager` window to install them; this requires an active Notebook session with Internet enabled, though note that you'll have to then disable Internet if you want to submit to a competition. When you Save your Notebook, your dependencies will be installed prior to the Save execution, even if your Notebook has Internet disabled.

When your exported Package is imported elsewhere it will automatically run the Dependency Manager's installation script which installs its saved package archives.

#### 3. Package Validation

When you Save a Notebook which exports a Package, we perform some validation on that Package. This step checks for several things:

- **Import:** Ensures that we can `import` your Package without errors.
- **Create a Model:** If your Package has a `class Model` defined, we will create an instance of it to ensure that succeeds.
- **Competition-Specific Checks (if applicable):** If your Package is intended for a code competition, we run that competition's `kaggle_evaluation.test` function with your `Model` to check whether you're following the expected input/output format.
- **Create a Model:** If your Package has a `class Model` defined, we will create an instance of it to ensure that succeeds.
- **Dependency tracking:** We track all the `kagglehub` dependencies requested by your Package during the above steps and write the versions used into `kagglehub_requirements.yaml`. This helps promote reproducibility, so that later re-use of your Package will use the same versions of those dependencies instead of silently taking newer versions which could cause breakage or altered behavior.

If validation fails, you'll see error messages in the saved Notebook's Output tab. You'll need to fix the issues and Save a new version. One common error case may be that your Notebook `import`ed a required package which worked in your Interactive session, but that `import` statement was not exported to your Package via the `nbdev` tag `#| export`.

***

### Importing a Package

You can import a Kaggle Package using `kagglehub.package_import()` in Kaggle Notebooks, Colab, your local machine, or anywhere you have `kagglehub` installed. See the `kagglehub` [homepage](https://github.com/Kaggle/kagglehub) for more details, including how to login with your Kaggle credentials which will be required to access private resources.

```codeBlock
import kagglehub

# Import the package (replace with the actual handle)
package = kagglehub.package_import('user/notebook-name') # Take latest version
# OR take specific version
package = kagglehub.package_import('user/notebook-name/versions/123')

# Use the package, calling whatever code it had defined, for example:
model = package.Model()
result = model.predict(...)
```

#### Docker

When running Packages on your own machines, we highly recommended you use Docker. Using the correct Docker image ensures that the Package has the same system dependencies as when it was created. It also provides a sandboxed environment, isolating your code from your main system when running code which can alter your python environment by installing dependencies, or could be untrusted code altogether.

First install [Docker](https://www.docker.com/get-started/) on your machine. Then find your target Package's docker image tag in the Package's `package/__init__.py` file's `__docker_image__` metadata. For example you might find

```codeBlock
__docker_image__ = 'gcr.io/kaggle-images/python@sha256:abcxyz...'
```

Then run `docker pull gcr.io/...` (replacing with the correct tag value) to download the image to your machine. NOTE: our images are over 20 GB in size. Then run the following to create a Container and enter a shell to start working within it:

```codeBlock
docker run -it --rm \
  gcr.io/... \
  /bin/bash
```

Again make sure to replace with the correct image tag. You may consider other arguments such as:

- **`--gpus all`:** This gives the Docker Container access to your machine's GPU(s) which may be required for some Packages to work. You can also provide more fine-grained access.
- **`-v /path/on/your/host:/path/in/container:ro`:** This links a directory on your host machine to a directory within your Docker Container, for example if you want to access your own data file to pass new inputs to the Package within the Container. Note the `:ro` piece provides read-only permission to the Container, but that can be dropped to enable write permission if desired, but be careful when doing so while running potentially untrusted code.
- **`--name your-container-name`:** This provides a custom name to refer to your Container instead of an auto-generated name.

You should now have a shell session inside your Container where you can run `python` and access Packages via `kagglehub.package_import`. As mentioned above, you'll need to login to your Kaggle account to access private resources.

Note that our Docker Images are updated every few weeks, generally with only modest updates between each. In many cases a Package could still work on somewhat older or newer Images relative to the precise one on which it was saved, and you could try this rather than downloading several of our large Images separately. One caveat is we have two "branches" of Images, one for CPU sessions and one for GPU sessions, and you should take care to use the right one. See our repositories of [CPU-based Images](http://gcr.io/kaggle-images/python) and [GPU-based Images](http://gcr.io/kaggle-gpu-images/python).

#### GPUs

Packages can use GPUs for their model inference and we tag this as metadata `__gpus__ = ...` within the exported Package's `__init__.py` file. Such Packages may fail when run without GPU(s), or even without the precise GPU configuration it was created with.

***

### Submitting to a Package Competition

To submit to a Kaggle Code Competition that uses Packages, follow these steps:

1. **Join the Competition:** Make sure you've joined the competition.
2. **Create a Package Notebook:** Create a Kaggle Notebook that defines your Package, following the instructions in the "Creating a Package" section. Your Notebook must be attached to the competition, for example by using the `New Notebook` button on the competition's `Code` page.
3. **Follow Competition Package format:** Your Package must define a `class Model` with a `predict()` method. This method must accept the correct input type(s) and returns the correct output type(s), as specified by the competition.
4. **(Optional) Test your Package:** Use the competition's provided `kaggle_evaluation.test(Model)` function to test that your `Model` returns appropriate responses. The Package's Validation run at Save time will also run this.
5. **Save Version and Submit:** "Save Version" of your notebook. Once it's finished running and validation passes, go to the Output tab of your saved version and click `Submit to Competition`.
6. **(Shortcut) Submit from the Notebook Editor:** The Competition panel in the Editor sidebar lets you Submit directly from the Editor, which automatically combines the Save Version and Submit steps sequentially.
7. **Review Competition Documentation:** Carefully read the competition's documentation pages for any specific rules or constraints which are required of you.

Kaggle's submission system will then run a hidden scoring session where we import your Package, instantiate a `Model` instance, iterate over the competition's hidden test set and call your model's `predict` function over each test batch, then aggregate your predictions and calculate your score using the competition's evaluation metric.

Your scoring session will use the same Notebook Accelerator (GPU) which was configured in the Kaggle Notebook which generated your Package. Future improvements may decouple this.

Like any Kaggle Code Competition, we intentionally limit the information you can obtain about your scoring session to discourage exfiltration of information about the hidden test set. See more information and debugging tips [here](https://www.kaggle.com/code-competition-debugging).

***

### Known Issues

The following is an incomplete list of limitations with current Package functionality:

1. **Cannot reference older versions of Notebook or Package datasources:** Our Notebook Editor will allow you to attach older versions of Notebook datasources (which includes Packages) via `kagglehub.notebook_output_download('user/notebook/versions/123')` or `kagglehub.package_import('user/notebook/versions/123')` and that will work in an Interactive session. However, when you Save your Notebook we auto-attach the latest version and that `kagglehub` command will unfortunately fail.
2. **Pin via Editor UI for older versions of Dataset datasources:** The same behavior described above applies to Dataset datsources, except that we support a workaround by Pinning your datasource. Once you attach your older Dataset, you can use the right-hand sidebar to Pin it to that older version which will be honored when Saving your Notebook.
3. **Cannot reference multiple versions of Datasets or Notebooks:**Model datasources support multi-versioning, but other types currently do not.
4. **Utility Scripts not supported:**Unfortunately the `kagglehub.utility_script_install` command is not supported within a Kaggle Notebook and cannot be used in your Package. However, Packages offer (mostly) a superset of Utility Script behavior, so you may consider converting your Utility Script into a Package and importing that instead.
5. **Cannot nest a Package into itself:**You may define your Package to utilize another Package inside it (nesting), but you cannot refer to the Notebook in which you're currently working or older versions of it.
6. **All required datasources must be attached before Save:** When we execute your Notebook at Save time, you are not allowed to attach any datasource version which was not already attached, so you must make sure your Notebook has everything attached before saving. One preferred pattern is for your `class Model` to have a `def __init__(self)` constructor which retrieves all required dependencies, and you execute a (non-exported) cell with `model = Model()` (or even run the competition's `kaggle_evaluation.test` function) in your Interactive Notebook session which should run your `kagglehub` commands to pull in your required dependencies. This also lets you see if there are errors and correct them before you try to Save.
7. **`kagglehub_requirements.yaml` inference is imperfect:**We auto-generate the `package/kagglehub_requirements.yaml` file which lists all Kaggle resources which your Package requested via corresponding `kagglehub` calls, along with the version used when your Package was created. Then when your Package is imported later it tries to use those same versions again, encouraging better reproducibility for your saved Package artifact. However, the auto-generation process comes from executing your Package during the Validation Run at Save time and might not capture all possible `kagglehub` calls that your code could make. For example, if you conditionally request a resource and the Validation Run didn't trigger it, we don't know that your Package needs it. The **best practice** here is to retrieve all your resources within the top-level Package code (which gets run on `import` or within your `class Model`'s `__init__` constructor, which also gets run at Validation time.
8. **Compute settings coupled between source Notebook and exported Package:**We mark the `__gpus__` Package metadata based on the Accelerator settings of the source Notebook, and a competition Package will use those same settings for its scoring run. Ideally we'd support decoupling those, so a source Notebook can have different settings than those intended for its exported Package.
9. **Packages with Dependency Manager require Unix-like systems:**Dependency Manager currently assumes it is run on Unix-like systems (using a `.sh` script) and will not work on other platforms like Windows. We'd like to fix this, but you can also follow the guidance above on running the Package within Kaggle's Docker Image which provides a Unix-like environment.

def nfgl_two(matrix):
    """
    Computes NFGL-2 (Normalized Frobenius-like Form / Frobenius Normal Form / Rational Canonical Form).
    
    Given a matrix A in F_q^(n x n), this algorithm returns:
    - companion_matrices: A list of companion matrices C(f_i) for the invariant factors.
    - invariant_factors: The monic polynomials f_1, f_2, ..., f_k such that f_1 | f_2 | ... | f_k.
    """
    # 1. Compute invariant factors of matrix A
    # 2. Construct companion matrices C(p) for each invariant factor p(x)
    # 3. Form the block diagonal matrix of companion matrices
    pass


def fgl_one(matrix):
    """
    Computes FGL-1 (First Frobenius Normal Form / Primary Rational Canonical Form).
    
    Decomposes the characteristic polynomial into power of irreducible factors (elementary divisors)
    and constructs the block diagonal matrix of their companion matrices.
    """
    pass


# Example Usage:
if __name__ == "__main__":
    import numpy as np

    # Example 3x3 matrix
    A = np.array([[1, 2, 0], [0, 1, 3], [0, 0, 2]])

    print("Matrix A:")
    print(A)


import math
from mpmath import mp

# 118 Elements ordered by atomic number (1 to 118)
ELEMENTS = [
    "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne",
    "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca",
    "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn",
    "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr",
    "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn",
    "Sb", "Te", "I", "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd",
    "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb",
    "Lu", "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg",
    "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th",
    "Pa", "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm",
    "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds",
    "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og"
]

def get_pi_digits(n_digits: int) -> str:
    """Generates pi digits without decimal point."""
    mp.dps = n_digits + 2
    pi_str = str(mp.pi).replace(".", "")
    return pi_str[:n_digits]

def pi_element_sequence(length: int = 10, chunk_size: int = 2) -> list[dict]:
    """
    Reads digits of Pi in chunks to map directly to atomic numbers (1–118).
    """
    digits_needed = length * chunk_size
    pi_digits = get_pi_digits(digits_needed)
    sequence = []

    for i in range(0, digits_needed, chunk_size):
        chunk = pi_digits[i:i + chunk_size]
        val = int(chunk)
        
        # Map values 00-99 into 1-118 range using modulo
        atomic_num = (val % len(ELEMENTS)) + 1
        symbol = ELEMENTS[atomic_num - 1]
        
        sequence.append({
            "pi_digits": chunk,
            "atomic_number": atomic_num,
            "symbol": symbol
        })

    return sequence

# Example Usage: Generate first 10 elements derived from Pi
results = pi_element_sequence(length=10, chunk_size=2)
for item in results:
    print(f"Pi Digits: {item['pi_digits']} -> Atomic No. {item['atomic_number']}: {item['symbol']}")
