.PHONY: all build test clean

# ------------------------------------------------------------------------------
# Variables
# ------------------------------------------------------------------------------
CXX       = g++
# On ajoute -I/opt/homebrew/include pour que boost/config.hpp soit trouvé.
# Aussi, on inclut -g0 -O3 pour l'optimisation, et -std=c++17 pour être sûr.
CXXFLAGS += -I/opt/homebrew/include -g0 -O3 -std=c++17

# Si besoin, on ajoute -L/opt/homebrew/lib au chemin de librairies
LDFLAGS  += -L/opt/homebrew/lib

# On récupère les flags fournis par quantlib-config. 
# NB: --cflags peut aussi inclure -I..., et --libs inclut -lQuantLib.
QL_CFLAGS = $(shell quantlib-config --cflags)
QL_LIBS   = $(shell quantlib-config --libs)

# ------------------------------------------------------------------------------
# Règles de construction
# ------------------------------------------------------------------------------
all: build test

build: main

test: main
	./main

# ------------------------------------------------------------------------------
# Cible principale
# ------------------------------------------------------------------------------
main: *.hpp *.cpp
	$(CXX) $(CXXFLAGS) $(QL_CFLAGS) *.cpp $(LDFLAGS) $(QL_LIBS) -o $@

# ------------------------------------------------------------------------------
# Cible de nettoyage
# ------------------------------------------------------------------------------
clean:
	rm -f main
