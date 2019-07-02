#include "tinyobjloader/objLoader.h"


namespace objLoader {

#define TINYOBJ_SSCANF_BUFFER_SIZE  (4096)

struct vertex_index {
    int v_idx, vt_idx, vn_idx;
    vertex_index(){
        v_idx = -1;
        vt_idx = -1;
        vn_idx = -1;
    }
    vertex_index(int idx) : v_idx(idx), vt_idx(idx), vn_idx(idx){}
    vertex_index(int vidx, int vtidx, int vnidx)
        : v_idx(vidx), vt_idx(vtidx), vn_idx(vnidx){}
};


// for std::map
static inline bool operator<(const vertex_index &a, const vertex_index &b) {
    if (a.v_idx != b.v_idx)
        return (a.v_idx < b.v_idx);
    if (a.vn_idx != b.vn_idx)
        return (a.vn_idx < b.vn_idx);
    if (a.vt_idx != b.vt_idx)
        return (a.vt_idx < b.vt_idx);
    return false;
}

static inline bool isSpace(const char c) { return (c == ' ') || (c == '\t'); }

static inline bool isNewLine(const char c) {
    return (c == '\r') || (c == '\n') || (c == '\0');
}

// Make index zero-base, and also support relative index.
static inline int fixIndex(int idx, int n = 0) {
    if (idx > 0) return idx - 1;
    if (idx == 0) return 0;
    return n + idx; // negative value = relative
}


// Tries to parse a floating point number located at s.
//
// s_end should be a location in the string where reading should absolutely
// stop. For example at the end of the string, to prevent buffer overflows.
//
// Parses the following EBNF grammar:
//   sign    = "+" | "-" ;
//   END     = ? anything not in digit ?
//   digit   = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
//   integer = [sign] , digit , {digit} ;
//   decimal = integer , ["." , integer] ;
//   float   = ( decimal , END ) | ( decimal , ("E" | "e") , integer , END ) ;
//
//  Valid strings are for example:
//   -0	 +3.1417e+2  -0.0E-3  1.0324  -1.41   11e2
//
// If the parsing is a success, result is set to the parsed value and true 
// is returned.
//
// The function is greedy and will parse until any of the following happens:
//  - a non-conforming character is encountered.
//  - s_end is reached.
//
// The following situations triggers a failure:
//  - s >= s_end.
//  - parse failure.
// 
static bool tryParseDouble(const char *s, const char *s_end, double *result)
{
	if (s >= s_end){
		return false;
	}

	double mantissa = 0.0;
	// This exponent is base 2 rather than 10.
	// However the exponent we parse is supposed to be one of ten,
	// thus we must take care to convert the exponent/and or the 
	// mantissa to a * 2^E, where a is the mantissa and E is the
	// exponent.
	// To get the final double we will use ldexp, it requires the
	// exponent to be in base 2.
	int exponent = 0;

	// NOTE: THESE MUST BE DECLARED HERE SINCE WE ARE NOT ALLOWED
	// TO JUMP OVER DEFINITIONS.
	char sign = '+';
	char exp_sign = '+';
	char const *curr = s;

	// How many characters were read in a loop. 
	int read = 0;
	// Tells whether a loop terminated due to reaching s_end.
	bool end_not_reached = false;

	/*
		BEGIN PARSING.
	*/

	// Find out what sign we've got.
	if (*curr == '+' || *curr == '-')
	{
		sign = *curr;
		curr++;
	}
	else if (isdigit(*curr)) { /* Pass through. */ }
	else
	{
		goto fail;
	}

	// Read the integer part.
	while ((end_not_reached = (curr != s_end)) && isdigit(*curr))
	{
		mantissa *= 10;
		mantissa += static_cast<int>(*curr - 0x30);
		curr++;	read++;
	}

	// We must make sure we actually got something.
	if (read == 0)
		goto fail;
	// We allow numbers of form "#", "###" etc.
	if (!end_not_reached)
		goto assemble;

	// Read the decimal part.
	if (*curr == '.')
	{
		curr++;
		read = 1;
		while ((end_not_reached = (curr != s_end)) && isdigit(*curr))
		{
			// NOTE: Don't use powf here, it will absolutely murder precision.
			mantissa += static_cast<int>(*curr - 0x30) * pow(10.0, -read);
			read++; curr++;
		}
	}
	else if (*curr == 'e' || *curr == 'E') {}
	else
	{
		goto assemble;
	}

	if (!end_not_reached)
		goto assemble;

	// Read the exponent part.
	if (*curr == 'e' || *curr == 'E')
	{
		curr++;
		// Figure out if a sign is present and if it is.
		if ((end_not_reached = (curr != s_end)) && (*curr == '+' || *curr == '-'))
		{
			exp_sign = *curr;
			curr++;
		}
		else if (isdigit(*curr)) { /* Pass through. */ }
		else
		{
			// Empty E is not allowed.
			goto fail;
		}

		read = 0;
		while ((end_not_reached = (curr != s_end)) && isdigit(*curr))
		{
			exponent *= 10;
			exponent += static_cast<int>(*curr - 0x30);
			curr++;	read++;
		}
		exponent *= (exp_sign == '+'? 1 : -1);
		if (read == 0)
			goto fail;
	}

assemble:
	*result = (sign == '+'? 1 : -1) * ldexp(mantissa * pow(5.0, exponent), exponent);
	return true;
fail:
	return false;
}
static inline float parseFloat(const char *&token) {
    token += strspn(token, " \t");
#ifdef TINY_OBJ_LOADER_OLD_FLOAT_PARSER
    float f = (float)atof(token);
    token += strcspn(token, " \t\r");
#else
    const char *end = token + strcspn(token, " \t\r");
    double val = 0.0;
    tryParseDouble(token, end, &val);
    float f = static_cast<float>(val);
    token = end;
#endif
    return f;
}


static inline void parseFloat2(float &x, float &y, const char *&token) {
    x = parseFloat(token);
    y = parseFloat(token);
}

static inline void parseFloat3(float &x, float &y, float &z, const char *&token) 
{
    x = parseFloat(token);
    y = parseFloat(token);
    z = parseFloat(token);
}

// Parse triples: i, i/j/k, i//k, i/j
static vertex_index parseTriple(const char *&token) {
    vertex_index vi(-1);

    vi.v_idx = fixIndex(atoi(token), 0);
    token += strcspn(token, "/ \t\r");
    if (token[0] != '/') {
        return vi;
    }
    token++;

    // i//k
    if (token[0] == '/') {
        token++;
        vi.vn_idx = fixIndex(atoi(token), 0);
        token += strcspn(token, "/ \t\r");
        return vi;
    }

    // i/j/k or i/j
    vi.vt_idx = fixIndex(atoi(token), 0);
    token += strcspn(token, "/ \t\r");
    if (token[0] != '/') {
        return vi;
    }

    // i/j/k
    token++; // skip '/'
    vi.vn_idx = fixIndex(atoi(token), 0);
    token += strcspn(token, "/ \t\r");
    return vi;
}

bool LoadObj(shape_t& shape, const std::string& filename) 
{
    std::ifstream inStream(filename );
    if ( !inStream){
        std::cout<<"Can not open files "<<filename<<std::endl;
        return false;
    }
    
    int matId = -1;
    int maxchars = 8192;             // Alloc enough size.
    std::vector<char> buf(static_cast<size_t>(maxchars)); // Alloc enough size.
    while (inStream.peek() != -1) {
        inStream.getline(&buf[0], maxchars);

        std::string linebuf(&buf[0]);

        // Trim newline '\r\n' or '\n'
        if (linebuf.size() > 0) {
            if (linebuf[linebuf.size() - 1] == '\n')
                linebuf.erase(linebuf.size() - 1);
        }
        if (linebuf.size() > 0) {
            if (linebuf[linebuf.size() - 1] == '\r')
                linebuf.erase(linebuf.size() - 1);
        }

        // Skip if empty line.
        if (linebuf.empty()) {
            continue;
        }

        // Skip leading space and tab
        const char *token = linebuf.c_str();
        token += strspn(token, " \t");

        assert(token);
        if (token[0] == '\0')
            continue; // empty line

        if (token[0] == '#')
            continue; // comment line

        // vertex
        if (token[0] == 'v' && isSpace((token[1]))) {
            token += 2;
            float x, y, z;
            parseFloat3(x, y, z, token);
            shape.mesh.positions.push_back(x);
            shape.mesh.positions.push_back(y);
            shape.mesh.positions.push_back(z);
            continue;
        }

        // normal
        if (token[0] == 'v' && token[1] == 'n' && isSpace((token[2]))) {
            token += 3;
            float x, y, z;
            parseFloat3(x, y, z, token);
            shape.mesh.normals.push_back(x);
            shape.mesh.normals.push_back(y);
            shape.mesh.normals.push_back(z);
            continue;
        }

        // texcoord
        if (token[0] == 'v' && token[1] == 't' && isSpace((token[2]))) {
            token += 3;
            float x, y;
            parseFloat2(x, y, token);
            shape.mesh.texcoords.push_back(x);
            shape.mesh.texcoords.push_back(y);
            continue;
        }

        // face
        if (token[0] == 'f' && isSpace((token[1]))) {
            token += 2;
            token += strspn(token, " \t");
            
            int cnt = 0;
            while (!isNewLine(token[0])) {
                cnt++;
                vertex_index vi =
                    parseTriple(token );
                shape.mesh.indicesP.push_back(vi.v_idx );
                shape.mesh.indicesN.push_back(vi.vn_idx );
                shape.mesh.indicesT.push_back(vi.vt_idx );
                size_t n = strspn(token, " \t\r");
                token += n;
            }
            if(cnt > 3){
                std::cout<<"Wrong: can not handle polygon with more than 3 edges"<<std::endl;
                return false;
            }
            shape.mesh.materialIds.push_back(matId);
            continue;
        }

        // use mtl
        if ((0 == strncmp(token, "usemtl", 6)) && isSpace((token[6]))) {
            char namebuf[TINYOBJ_SSCANF_BUFFER_SIZE];
            token += 7;
#ifdef _MSC_VER
            sscanf_s(token, "%s", namebuf, (unsigned)_countof(namebuf));
#else
            sscanf(token, "%s", namebuf);
#endif  
            bool isFind = false;
            for(int i = 0; i < shape.mesh.materialNames.size(); i++){
                if(shape.mesh.materialNames[i] == std::string(namebuf) ){
                    isFind = true;
                    matId = i;
                }
            }
            if(isFind == false){
                std::cout<<std::string(namebuf )<<std::endl;
                shape.mesh.materialNames.push_back(std::string(namebuf ) );
                shape.mesh.materialNameIds.push_back(0);
                matId = shape.mesh.materialNames.size() - 1;
            }
            continue;
        }
    }  
    return true;
} // namespace

}
