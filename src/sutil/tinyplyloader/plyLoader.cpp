#include "tinyplyloader/plyLoader.h"

namespace plyLoader{

bool LoadPly(shape_t& shape, const std::string& filename)
{
	try
	{
		std::ifstream ss(filename, std::ios::binary);
		if (ss.fail() ) throw std::runtime_error(std::string("failed to open ") + filename);

        tinyply::PlyFile file;
		file.parse_header(ss);

		std::cout << "........................................................................\n";
		for (auto c : file.get_comments()) std::cout << "Comment: " << c << std::endl;
		for (auto e : file.get_elements() )
		{
			std::cout << "element - " << e.name << " (" << e.size << ")" << std::endl;
			for (auto p : e.properties) 
                std::cout << "\tproperty - " << p.name << " (" << tinyply::PropertyTable[p.propertyType].str << ")" << std::endl;
		}
		std::cout << "........................................................................\n";

		// Tinyply treats parsed data as untyped byte buffer. See below for examples.
		std::shared_ptr<tinyply::PlyData> vertices = NULL, normals = NULL, textures = NULL;
        std::shared_ptr<tinyply::PlyData> vertexIndices = NULL, texcoords = NULL;

		// The header information can be used to programmatically extract properties on elements
		// known to exist in the header prior to reading the data. For brevity of this sample, properties 
		// like vertex position are hard-coded: 
		try { 
            vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); 
        }
		catch (const std::exception & e) { 
            std::cerr << "plyLoader exception: " << e.what() << std::endl; 
            return false; 
        }

		try { 
            normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" });
        }
		catch (const std::exception & e) {
            std::cout<<"Warning: plyLoader can not get vertex normals"<<std::endl;
        }

		try { 
            textures = file.request_properties_from_element("vertex", { "u", "v" }); 
        }
		catch (const std::exception & e) { 
            std::cout<<"Warning: plyLoader can not get vertex textures"<<std::endl;
        }

		try { 
            vertexIndices = file.request_properties_from_element("face", { "vertex_indices" }, 3); 
        }
		catch (const std::exception & e) { 
            std::cerr << "plyLoader exception: " << e.what() << std::endl;
            return false;
        }
		
        try { 
            texcoords = file.request_properties_from_element("face", { "texcoord" }, 2); 
        }
		catch (const std::exception & e) { 
            std::cout<<"Warning: plyLoader can not get texture coordinates"<<std::endl;
        }

		file.read(ss);


		if (vertices) {
            std::cout << "\tRead " << vertices->count << " total vertices "<< std::endl;
        }
		if (normals){
            assert(normals->count == vertices -> count);
        }
        if (textures ) {
            assert(textures -> count == vertices -> count );
        }
		if (vertexIndices ){
            std::cout << "\tRead " << vertexIndices->count << " total faces (triangles) " << std::endl;
        }
		if (texcoords) {
            assert(texcoords -> count == vertexIndices -> count );
            if(textures != NULL){
                std::cout<<"Warning: only one type of texture coordinates are accepted!"<<std::endl;
                textures = NULL;
            }
        }

        if(vertices != NULL){
		    const size_t numBytes = vertices->buffer.size_bytes();
            assert(numBytes == vertices->count * 3 * 4);
		    shape.mesh.positions.resize(vertices->count * 3, 0.0);
		    std::memcpy(shape.mesh.positions.data(), vertices->buffer.get(), numBytes );
        }

        if(normals != NULL){
            const size_t numBytes = normals -> buffer.size_bytes();
            assert(numBytes == normals->count * 3 * 4);
            shape.mesh.normals.resize(normals->count * 3, 0.0);
            std::memcpy(shape.mesh.normals.data(), normals->buffer.get(), numBytes );
        }

        if(textures != NULL){
            const size_t numBytes = textures -> buffer.size_bytes();
            assert(numBytes == textures -> count * 2 * 4);
            shape.mesh.texcoords.resize(textures->count * 2, 0.0);
            std::memcpy(shape.mesh.texcoords.data(), textures->buffer.get(), numBytes );
        }

        if(vertexIndices != NULL){
            const size_t numBytes = vertexIndices -> buffer.size_bytes();
            assert(numBytes == vertexIndices -> count * 3 * 4);
            shape.mesh.indicesP.resize(vertexIndices -> count * 3);
            std::memcpy(shape.mesh.indicesP.data(), vertexIndices->buffer.get(), numBytes);
            if(normals != NULL){
                shape.mesh.indicesN.resize(vertexIndices -> count * 3);
                std::memcpy(shape.mesh.indicesN.data(), vertexIndices->buffer.get(), numBytes);
            }
            else{
                shape.mesh.indicesN.erase(shape.mesh.indicesN.begin(), 
                        shape.mesh.indicesN.end() );
                shape.mesh.indicesN.resize(vertexIndices -> count * 3, -1);
            }

            if(textures != NULL){
                shape.mesh.indicesT.resize(vertexIndices -> count * 3);
                std::memcpy(shape.mesh.indicesT.data(), vertexIndices->buffer.get(), numBytes);
            }
            else if(textures == NULL && texcoords == NULL){
                shape.mesh.indicesT.erase(shape.mesh.indicesT.begin(), 
                        shape.mesh.indicesT.end() );
                shape.mesh.indicesT.resize(vertexIndices -> count * 3, -1);      
            }
            
            // Currently .ply format does not have a standard to support materials 
            shape.mesh.materialIds.erase(shape.mesh.materialIds.begin(), 
                    shape.mesh.materialIds.end() );
            shape.mesh.materialIds.resize(vertexIndices -> count * 3, -1);

        }

        if(texcoords != NULL){
            const size_t numBytes = texcoords -> buffer.size_bytes();
            assert(numBytes == texcoords -> count * 6 * 4);
            shape.mesh.texcoords.resize(texcoords -> count * 6);
            shape.mesh.indicesT.resize(texcoords -> count * 3);
            std::memcpy(shape.mesh.texcoords.data(), texcoords -> buffer.get(), numBytes );
            for(int i = 0; i < texcoords -> count * 3; i++)
                shape.mesh.indicesT[i] = i;
        }
	}
	catch (const std::exception & e)
	{
		std::cerr << "Caught plyLoader exception: " << e.what() << std::endl;
        return false;
	}

    return true; 
}

}
