<img src="https://mcdim.xyz/media/lykan0.png" alt="lykan logo" style="height: 130px;"/>

# lykan
Lykan is a leak analysis program written in C that generates visual graphs of the analysed data. It receives as inputs raw password text dumps where the passwords are separated by '\n' and extracts statistical information about the dumps, including the top passwords, character popularity etc. Lykan is meant to help understand the psychology & the trends of password-making by the average user, information which can be later used defensively or offensively for security assessment.

Under development

# Installation

To install lykan, just run the following commands in the shell:
```
$ git clone install https://github.com/michaeldim02/lykan && cd lykan/src
$ sudo make install
```

## Dependencies
To build:
* gcc / clang / tcc
* Make

To run:
* gnuplot
