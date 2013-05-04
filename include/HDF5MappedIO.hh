//////////////////////////////////////////////////////////////////////////
// G4VoxelData
// ===========
// A general interface for loading voxelised data as geometry in GEANT4.
//
// Author:  Christopher M Poole <mail@christopherpoole.net>
// Source:  http://github.com/christopherpoole/G4VoxelData
//
// License & Copyright
// ===================
// 
// Copyright 2013 Christopher M Poole <mail@christopherpoole.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////


#ifndef HDF5MAPPEDIO_H
#define HDF5MAPPEDIO_H

// G4VOXELDATA //
#include "G4VoxelArrayMappedIO.hh"

// HDF5 //
#include "H5Cpp.h"

// GEANT4 //
#include "globals.hh"


template <typename T>
//class HDF5MappedIO : public G4VoxelArrayMappedIO<T> {
class HDF5MappedIO : public G4VoxelArray<T> {
  public:
    using G4VoxelArray<T>::Init;

   HDF5MappedIO<T>() {
   };
  
  public:
    void Read(G4String filename, G4String dataset_name) {
        file = H5::H5File(filename.c_str(), H5F_ACC_RDONLY);
        dataset = file.openDataSet(dataset_name.c_str());
        dataspace = dataset.getSpace();

        this->ndims = dataspace.getSimpleExtentNdims();
        
        hsize_t shape[this->ndims];
        dataspace.getSimpleExtentDims(shape);
        this->shape.assign(shape, shape + this->ndims);
        
        H5::DSetCreatPropList header = dataset.getCreatePlist();
        if (H5D_CHUNKED == header.getLayout())  {
            hsize_t shape[this->ndims];
            int chunk_ndims = header.getChunk(32, shape);
            this->buffer_shape.assign(shape, shape + this->ndims);
         } else {
            // TODO assign buffer shape if data not chunked
         }

        this->length = 1;
        for (int i=0; i<this->ndims; i++) this->length *= this->shape[i];

        Init();
    };

    T GetValue(unsigned int index) {
        unsigned int z = index / (this->shape[0] * this->shape[1]);

        unsigned int sub_index = index % (this->shape[0] * this->shape[1]);
        unsigned int y = sub_index / this->shape[0];
        unsigned int x = sub_index % this->shape[0];
        
        return GetValue(x, y, z);
    };

    T GetValue(unsigned int x, unsigned int y, unsigned int z) {
        if (this->ndims != 3) {
            // TODO raise exception if not 3D dataset
        }

        hsize_t offset[this->ndims];   // hyperslab offset in the file
        // Round to nearest buffer shape (only works for unsinged int's)
        offset[0] = (x / this->buffer_shape[0]) * this->buffer_shape[0];
        offset[1] = (y / this->buffer_shape[1]) * this->buffer_shape[1];
        offset[2] = (z / this->buffer_shape[2]) * this->buffer_shape[2];

        // Make a window into the data on disk
        dataspace.selectHyperslab(H5S_SELECT_SET, &(this->buffer_shape)[0], offset);

        // Make a map to the disk window
        memspace = H5::DataSpace(3, &(this->buffer_shape)[0]);

        // Make a window into the map in memory
        hsize_t  offset_out[3] = {0, 0, 0};  // hyperslab offset in memory (none)
        memspace.selectHyperslab( H5S_SELECT_SET, &(this->buffer_shape)[0], offset_out );

        // Populate memory with the data seen through the disk window. HDF5 should
        // transparently cache data here? Reads from disk will only happen if required.
        T data_out[this->buffer_shape[0]][this->buffer_shape[1]][this->buffer_shape[2]];
        dataset.read(data_out, H5::PredType::NATIVE_INT, memspace, dataspace );
        
        // The value request will be in the memory window minus the offset
        // applied to the disk window.
        return data_out[x - offset[0]][y - offset[1]][z - offset[2]];
    };

    //virtual void Write(G4String, G4MappedVoxelArray*) {
    //    G4Exception("G4VoxelData::Write", "Writing data not implemented.",
    //            FatalException, "");
    //};

  private:
    H5::H5File file;
    H5::DataSet dataset; 
    H5::DataSpace dataspace;
    H5::DataSpace memspace;

    std::vector<hsize_t> buffer_shape;
};

#endif // HDF5MAPPEDIO_H

