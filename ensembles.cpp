/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2008, Tod D. Romo, Alan Grossfield
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/





#include <ensembles.hpp>


namespace loos {

  // Assume all groups are already sorted or matched...

  AtomicGroup averageStructure(const std::vector<AtomicGroup>& ensemble) {
    AtomicGroup avg = ensemble[0].copy();

    // First, zap our coords...
    int n = avg.size();
    int i;
    for (i=0; i<n; i++)
      avg[i]->coords() = GCoord(0.0, 0.0, 0.0);

    // Now, accumulate...
    std::vector<AtomicGroup>::const_iterator j;
    for (j = ensemble.begin(); j != ensemble.end(); j++) {
      for (i = 0; i<n; i++)
        avg[i]->coords() += (*j)[i]->coords();
    }

    for (i=0; i<n; i++)
      avg[i]->coords() /= ensemble.size();

    return(avg);
  }



  AtomicGroup averageStructure(const AtomicGroup& g, const std::vector<XForm>& xforms, pTraj traj) {
    AtomicGroup avg = g.copy();
    AtomicGroup frame = g.copy();
    int n = avg.size();
    for (int i=0; i<n; i++)
      avg[i]->coords() = GCoord(0.0, 0.0, 0.0);

    traj->rewind();
    uint tn = traj->nframes();
    
    // Safety check...
    if (tn != xforms.size())
      throw(std::runtime_error("Mismatch in number of frames in the trajectory and passed transforms for loos::averageStructure()"));

    for (uint j=0; j<tn; j++) {
      traj->readFrame(j);
      traj->updateGroupCoords(frame);
      frame.applyTransform(xforms[j]);
      for (int i=0; i<n; i++)
        avg[i]->coords() += frame[i]->coords();
    }

    for (int i=0; i<n; i++)
      avg[i]->coords() /= tn;

    return(avg);
  }




  boost::tuple<std::vector<XForm>,greal,int> iterativeAlignment(std::vector<AtomicGroup>& ensemble,
                                                                      greal threshold, int maxiter) {
    int iter = 0;
    int n = ensemble.size();
    greal rms;
    std::vector<XForm> xforms(n);
    AtomicGroup avg;

    // Start by aligning against the first structure in the ensemble
    AtomicGroup target = ensemble[0].copy();

    target.centerAtOrigin();

    do {
      for (int i = 0; i<n; i++) {
        GMatrix M = ensemble[i].alignOnto(target);
        xforms[i].premult(M);
      }

      avg = averageStructure(ensemble);
      rms = avg.rmsd(target);
      target = avg;
      iter++;
    } while (rms > threshold && iter <= maxiter );
  
    boost::tuple<std::vector<XForm>, greal, int> res(xforms, rms, iter);
    return(res);
  }




  boost::tuple<std::vector<XForm>, greal, int> iterativeAlignment(const AtomicGroup& g, pTraj traj,
                                                                        greal threshold, int maxiter) {
    std::vector<AtomicGroup> frames;

    traj->rewind();
    while (traj->readFrame()) {
      AtomicGroup frame = g.copy();
      traj->updateGroupCoords(frame);
      frames.push_back(frame);
    }

    boost::tuple<std::vector<XForm>, greal, int> result = iterativeAlignment(frames, threshold, maxiter);
    return(result);
  }

}
