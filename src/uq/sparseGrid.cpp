#include <math.h>
#include "sparseGrid.hpp"
#include <iostream>

namespace femus
{

    sparseGrid::sparseGrid (const unsigned &N, const unsigned &M, std::vector < std::vector < double > >  &samples)
    {

        _N = N;
        _M = M;
        _L = static_cast<unsigned> (log10 (M) + 1);

        _intervals.resize (_N);
        _hs.resize (_N);
        _nodes.resize (_N);
        _hierarchicalDofs.resize (_N);

        for (unsigned n = 0; n < _N; n++) {
            _intervals[n].resize (2);
            _hs[n].resize (_L);
            _nodes[n].resize (_L);
            _hierarchicalDofs[n].resize (_L);
            for (unsigned l = 0; l < _L; l++) {
                unsigned dofsFullGrid = static_cast<unsigned> (pow (2, l + 1) - 1);
                _nodes[n][l].resize (dofsFullGrid);
                unsigned dofsHierarchical;

                if (dofsFullGrid % 2 != 0) { //odd number
                    dofsHierarchical = static_cast<unsigned> ( ( (pow (2, l + 1) - 1) + 1.) * 0.5);
                }
                else { //even number
                    dofsHierarchical = static_cast<unsigned> ( ( (pow (2, l + 1) - 1)) * 0.5);
                }

                _hierarchicalDofs[n][l].resize (dofsHierarchical);

                std::cout << "dofsFullGrid = " << dofsFullGrid << " , " << "dofsHierarchical = " << dofsHierarchical << std::endl;

            }
        }

        double an;
        double bn;

        //BEGIN figuring out the one dimensional intervals, mesh sizes and node coordinates
        for (unsigned n = 0; n < _N; n++) {
            an = - 1.; //temporary left extremum of the n-th interval [an,bn]
            bn = 1.;   //temporary right extremum of the n-th interval [an,bn]
            for (unsigned m = 0; m < _M; m++) {
                if (samples[m][n] <= an) {
                    an = samples[m][n];
                }
                else if (samples[m][n] >= bn) {
                    bn = samples[m][n];
                }
            }

            _intervals[n][0] = an - 0.5; //the 0.5 is just to add some tolerance
            _intervals[n][1] = bn + 0.5;

            std::cout << "a = " << _intervals[n][0] << " , " << "b = " << _intervals[n][1] << std::endl;

            for (unsigned l = 0; l < _L; l++) {
                _hs[n][l] = fabs (_intervals[n][1] - _intervals[n][0]) / pow (2, l + 1); // mesh size of the n-th interval
                std::cout << "h[" << n << "][" << l << "]= " << _hs[n][l] << std::endl;
                unsigned hierarchicalCounter = 0;
                for (unsigned i = 0; i < _nodes[n][l].size(); i++) {
                    _nodes[n][l][i] = _intervals[n][0] + (i + 1) * _hs[n][l]; //coordinates of the nodes of the n-th interval
                    std::cout << "node[" << n << "][" << l << "][" << i << "]= " << _nodes[n][l][i] << std::endl;
                    if (i % 2 == 0) {
                        _hierarchicalDofs[n][l][hierarchicalCounter] = i;
                        hierarchicalCounter++;
                    }
                }
            }

        }

        for (unsigned n = 0; n < _N; n++) {
            for (unsigned l = 0; l < _L; l++) {
                for (unsigned i = 0; i < _hierarchicalDofs[n][l].size(); i++) {
                    std::cout << "_hierarchicalDofs[" << n << "][" << l << "][" << i << "] = " << _hierarchicalDofs[n][l][i] << " " ;
                }
                std::cout << std::endl;
            }
        }

        //END


        //BEGIN computation of the sparse grid index set

        //this is done by first computing the tensor product space of the complete space and then eliminating those indices that don't satisfy the requirement: | |_1 <= _L + N - 1 (this condition holds assuming all indices start from 1)


        //here we compute the tensor produc index set for the full grid
        std::vector < std::vector < unsigned > > Tp;

        ComputeTensorProductSet (Tp, _L, _N);

        unsigned tensorProductDim = Tp.size();

        //here we only consider the indices of Tp that satisfy the requirement
        unsigned indexCounter = 0;

        for (unsigned i = 0; i < tensorProductDim; i++) {

            unsigned sum = 0;

            for (unsigned j = 0; j < _N; j++) {
                sum += (Tp[i][j] + 1); //this is because our indices start from 0 but the requirement assumes they start from 1
            }

            if (sum <= _L + _N - 1) {

                _indexSetW.resize (indexCounter + 1);
                _indexSetW[indexCounter].resize (_N);

                for (unsigned j = 0; j < _N; j++) {
                    _indexSetW[indexCounter][j] = Tp[i][j];
                }
                indexCounter++;
            }
        }

        for (unsigned i = 0; i < _indexSetW.size(); i++) {
            for (unsigned j = 0; j < _N; j++) {
                std::cout << "_indexSetW[" << i << "][" << j << "]= " << _indexSetW[i][j];
            }
            std::cout << std::endl;
        }

        _numberOfWs = _indexSetW.size();


        //END


        //BEGIN construction of dofIdentifier

        _dofIdentifier.resize (_numberOfWs);

        std::cout << "-------------------------- Number of W sets = " << _numberOfWs <<  "-------------------------- " << std::endl;

        std::vector< unsigned > maxDofs (_numberOfWs);

        for (unsigned w = 0; w < _numberOfWs; w++) {
            unsigned identifiersOfW = 1;
            for (unsigned n = 0; n < _N; n++) {

                identifiersOfW *= _hierarchicalDofs[n][_indexSetW[w][n]].size();

                if (_nodes[n][_indexSetW[w][n]].size() >= maxDofs[w]) maxDofs[w] = _nodes[n][_indexSetW[w][n]].size();

            }

            _dofIdentifier[w].resize (identifiersOfW);
        }

        for (unsigned w = 0; w < _numberOfWs; w++) {
            if (maxDofs[w] < 2) maxDofs[w] = 2; //otherwise ComputeTensorProductSet doesn't work
            std::cout << "maxDofs[" << w << "] = " << maxDofs[w] << std::endl;
        }


        //Here we create the dofs for each W
        std::vector<std::vector< std::vector < unsigned > > > dofsW;
        dofsW.resize (_numberOfWs);
        std::vector < unsigned> counterW (_numberOfWs, 0);
        for (unsigned w = 0; w < _numberOfWs; w++) {

            std::cout << "--------------------------- w = " << w << " ---------------------- " << std::endl;

            std::vector < std::vector < unsigned > > Tw;
            ComputeTensorProductSet (Tw, maxDofs[w], _N);
            for (unsigned j = 0; j < Tw.size(); j++) {
                //now for each Tw[j]:
                //1. we need to check if it has to be included
                //2. if yes, we have to check if it has already been included
                //3. if not, include it, and move to the next one

                //1.
                std::vector< unsigned > satisfiesRequirements (_N, 0);
                for (unsigned n = 0; n < _N; n++) {
                    for (unsigned i = 0; i < _hierarchicalDofs[n][_indexSetW[w][n]].size(); i++) {

                        std::cout << "Tw[" << j << "][" << n << "]  = " << Tw[j][n] << " , " << "i = " << i << " ," << _hierarchicalDofs[n][_indexSetW[w][n]][i] << std::endl;

                        if (Tw[j][n] == _hierarchicalDofs[n][_indexSetW[w][n]][i]) {
                            satisfiesRequirements[n] = 1;
                            std::cout << "satisfiesRequirements[" << n << "] = " << satisfiesRequirements[n] << std::endl;
                        }
                    }
                }

                unsigned shouldBeThere = 0;
                for (unsigned n = 0; n < _N; n++) {
                    shouldBeThere += satisfiesRequirements[n];
                }

                std::cout << "--------------------------- done 1 ---------------------- " << std::endl;

                //2.
                if (shouldBeThere == _N) {

                    std::vector< unsigned> seeIfItIsThere (_N, 0);

                    for (unsigned i = 0; i < dofsW[w].size(); i++) {
                        for (unsigned n = 0; n < _N; n++) {
                            if (dofsW[w][i][n] == Tw[j][n]) seeIfItIsThere[n] = 1;
                        }
                    }

                    unsigned itIsThere = 0;
                    for (unsigned n = 0; n < _N; n++) {
                        itIsThere += seeIfItIsThere[n];
                    }

                    std::cout << "--------------------------- done 2 ---------------------- " << std::endl;

                    //3.
                    if (itIsThere != _N) {

                        dofsW[w].resize (counterW[w] + 1);
                        dofsW[w][counterW[w]].resize (_N);

                        for (unsigned n = 0; n < _N; n++) {
                            dofsW[w][counterW[w]][n] = Tw[j][n];
                        }

                        counterW[w] += 1;

                    }

                    std::cout << "--------------------------- done 3 ---------------------- " << std::endl;
                }
            }


            for (unsigned i = 0; i < dofsW[w].size(); i++) {
                for (unsigned n = 0; n < _N; n++) {
                    std::cout << "dofsW[" << w << "][" << i << "][" << n << "] = " << dofsW[w][i][n] << " ";
                }
                std::cout << std::endl;
            }

        }


        //now we need to store these dofs in _dofIdentifier
        for (unsigned w = 0; w < _numberOfWs; w++) {
            for (unsigned i = 0; i < _dofIdentifier[w].size(); i++) {
                _dofIdentifier[w][i].resize (_N);
                for (unsigned n = 0; n < _N; n++) {
                    _dofIdentifier[w][i][n].resize (3);
                    _dofIdentifier[w][i][n][0] = n;
                    _dofIdentifier[w][i][n][1] = _indexSetW[w][n];
                    _dofIdentifier[w][i][n][2] = dofsW[w][i][n];
                }
            }
        }

        //to erase (it is just a check)
        for (unsigned w = 0; w < _numberOfWs; w++) {
            std::cout << " ------------------------- w = " << w << " ------------------------- " <<std::endl;
            for (unsigned i = 0; i < _dofIdentifier[w].size(); i++) {
                std::cout << " ------------------------- i = " << i << " ------------------------- " <<std::endl;
                for (unsigned n = 0; n < _N; n++) {
                    std::cout << " ------------------------- dim = " << n << " ------------------------- " <<std::endl;
                    for (unsigned j = 0; j < 3; j++) {
                       std::cout << "_dofIdentifier[" << w <<"][" << i <<"]["<< n << "]["<< j <<"] = " << _dofIdentifier[w][i][n][j] << " " ;
                    }
                    std::cout<< std::endl;
                }
            }
        }

        //END

    }

    void sparseGrid::EvaluateOneDimensionalPhi (double &phi, const double &x, const unsigned &n, const unsigned &l, const unsigned &i, const bool &scale)
    {

        //i tells you that you are going to compute the phi associated with the i-th node

        double leftBoundOfSupport = _nodes[n][l][i] - _hs[n][l]; // this is xi - h
        double rightBoundOfSupport = _nodes[n][l][i] + _hs[n][l]; // this is xi + h

        if (x >= leftBoundOfSupport && x <= rightBoundOfSupport) {

            phi = 1 - fabs ( (x - _nodes[n][l][i]) / _hs[n][l]);

            if (scale == true) {  //this is to be used when building the nodal values of the PDF approximation
                phi /= _hs[n][l];
            }

        }

        else phi = 0.;
    }

    void sparseGrid::EvaluatePhi (double &phi, const std::vector <double> &x, std::vector < std::vector < unsigned > > identifier)
    {

        //identifier tells us what one dimensional phis to multiply in order to get the desired phi
        //identifier[n][0] = n (dim), identifier[n][1] = l (level), identifier[n][2] = i (node)

        std::vector < double > oneDimPhi (_N, 0.);

        phi = 1.;

        for (unsigned n = 0; n < _N; n++) {
            EvaluateOneDimensionalPhi (oneDimPhi[n], x[n], identifier[n][0], identifier[n][1], identifier[n][2], false);
            phi *= oneDimPhi[n];
        }

    }

    void sparseGrid::ComputeTensorProductSet (std::vector< std::vector <unsigned>> &Tp, const unsigned &T1, const unsigned &T2)
    {

        unsigned tensorProductDim = pow (T1, T2);

        Tp.resize (tensorProductDim);
        for (unsigned i = 0; i < tensorProductDim; i++) {
            Tp[i].resize (T2);
        }

        unsigned index = 0;
        unsigned counters[T2 + 1];
        memset (counters, 0, sizeof (counters));

        while (!counters[T2]) {

            for (unsigned j = 0; j < T2; j++) {
                Tp[index][j] = counters[T2 - 1 - j];
                std::cout << " Tp[" << index << "][" << j << "]= " << Tp[index][j] ;
            }
            std::cout << std::endl;
            index++;

            unsigned i;
            for (i = 0; counters[i] == T1 - 1; i++) {   // inner loops that are at maxval restart at zero
                counters[i] = 0;
            }
            ++counters[i];  // the innermost loop that isn't yet at maxval, advances by 1
        }

    }


}













