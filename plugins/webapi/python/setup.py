import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="veyon", # Replace with your own username
    version="0.1.0",
    author="Tobias Junghans",
    author_email="tobydox@veyon.io",
    description="Veyon WebAPI client",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/veyon/veyon",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.2',
)
