from setuptools import setup, find_packages

setup(
    name='IGTradingBot',
    version='0.1',
    author='Your Name',
    packages=find_packages(include=['igtrader', 'igtrader.*']),
)