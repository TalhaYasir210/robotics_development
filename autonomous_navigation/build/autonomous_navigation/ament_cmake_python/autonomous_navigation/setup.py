from setuptools import find_packages
from setuptools import setup

setup(
    name='autonomous_navigation',
    version='0.0.0',
    packages=find_packages(
        include=('autonomous_navigation', 'autonomous_navigation.*')),
)
