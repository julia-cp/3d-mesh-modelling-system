#include "mesh.h"

#ifdef _WIN32
#include <Windows.h>
#include "GL\glut.h"
#define M_PI 3.141592654
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

#include "math.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <iostream>
#include <vector>
#include "mesh.h"
#include <map>
#include <queue>
#include <iomanip>
#include <stack>

using namespace std;

bool isBoundaryEdgesVisible = false; // the visibility of boundary edges
bool isHighlight = false;
int randomSeed = 1;
vector<vector<int>> vertexTriangles; // an array of triangles that uses each vertices
                                     // vertexTriangles[i] is an array of triangles that uses vertex i

void myObjType::draw() {

	glEnable(GL_LIGHTING);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	glPushMatrix();
	double longestSide = 0.0;
	for (int i = 0; i < 3; i++)
		if ((lmax[i] - lmin[i]) > longestSide)
			longestSide = (lmax[i] - lmin[i]);
	glScalef(4.0 / longestSide, 4.0 / longestSide, 4.0 / longestSide);
	glTranslated(-(lmin[0] + lmax[0]) / 2.0, -(lmin[1] + lmax[1]) / 2.0, -(lmin[2] + lmax[2]) / 2.0);
    if (!isHighlight) {
        // Draw all triangles with one call
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(3, GL_DOUBLE, 0, vlist);
        glNormalPointer(GL_DOUBLE, 0, vnlist); // the vertex normal vectors
        glDrawElements(GL_TRIANGLES, tcount * 3, GL_UNSIGNED_INT, tlist[1]);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
	glDisable(GL_LIGHTING);
    
    if (isBoundaryEdgesVisible) {
        visualizeBoundaryEdges(); // draws the boundary edges
    }
    
    if (isHighlight) {
        highlightComponents();
    }
    
	glPopMatrix();
}

/* Write the current mesh to an OBJ file */
void myObjType::writeFile(char* filename)
{
    ofstream outFile(filename);
        if (!outFile.is_open()) {
            cout << "Error: Unable to open file " << filename << " for writing." << endl;
            return;
        }

        // Write vertices
        for (int i = 1; i <= vcount; i++) {
            outFile << "v " << vlist[i][0] << " " << vlist[i][1] << " " << vlist[i][2] << endl;
        }

        // Write faces
        for (int i = 1; i <= tcount; i++) {
            outFile << "f " << tlist[i][0] << " " << tlist[i][1] << " " << tlist[i][2] << endl;
        }

        cout << "Mesh written to " << filename << " successfully." << endl;
        outFile.close();
	}

void myObjType::readFile(char* filename)
{
    cout << "Opening " << filename << endl;
	ifstream inFile;
	inFile.open(filename);
	if (!inFile.is_open()) {
		cout << "We cannot find your file " << filename << endl;
		exit(1);
	}

	string line;
	int i, j;
	bool firstVertex = 1;
	double currCood;

	while (getline(inFile, line))
	{
		if ((line[0] == 'v' || line[0] == 'f') && line[1] == ' ')
		{
			if (line[0] == 'v')
			{
				vcount++;
				i = 1;
				const char* linec = line.data();
				for (int k = 0; k < 3; k++) { // k is 0,1,2 for x,y,z
					while (linec[i] == ' ') i++;
					j = i;
					while (linec[j] != ' ' && linec[j] != '\0') j++;
					currCood = vlist[vcount][k] = atof(line.substr(i, j - i).c_str());
					if (firstVertex)
						lmin[k] = lmax[k] = currCood;
					else {
						if (lmin[k] > currCood)
							lmin[k] = currCood;
						if (lmax[k] < currCood)
							lmax[k] = currCood;
					}
					i = j;
				}

				firstVertex = 0;
			}
			if (line[0] == 'f')
			{
				tcount++;
				i = 1;
				const char* linec = line.data();
				for (int k = 0; k < 3; k++) {
					while (linec[i] == ' ') i++;
					j = i;
					while (linec[j] != ' ' && linec[j] != '\\' && linec[j] != '\0') j++;
					tlist[tcount][k] = atof(line.substr(i, j - i).c_str());
					i = j;
					fnlist[tcount][k] = 0;
					while (linec[j] != ' ' && linec[j] != '\0') j++;

				}

			}


		}
	}

	/* Computing Normal Vectors */
    for (int n = 1; n <= tcount; n++) {
        int v1, v2, v3; // id of the vertices
        v1 = tlist[n][0];
        v2 = tlist[n][1];
        v3 = tlist[n][2];
        
        double u[3], v[3];
        
        for (int k = 0; k < 3; k++) {
            u[k] = vlist[v2][k] - vlist[v1][k]; // v2 - v1
            v[k] = vlist[v3][k] - vlist[v2][k]; // v3 - v2
        }
        
        double nx, ny, nz;
        // cross product
        nx = u[1] * v[2] - u[2] * v[1];
        ny = u[2] * v[0] - u[0] * v[2];
        nz = u[0] * v[1] - u[1] * v[0];
        
        double mag_n = sqrt(nx * nx + ny * ny + nz * nz); // magnitude of normal vector
        
        // normalize
        nlist[n][0] = nx / mag_n;
        nlist[n][1] = ny / mag_n;
        nlist[n][2] = nz / mag_n;
        
    }
    
    cout << endl << endl;
    cout << "No. of vertices: " << vcount << endl;
    cout << "No. of triangles: " << tcount << endl;
    computeStat();
    
    /* List the triangles around each vertex */
    vertexTriangles.assign(vcount + 1, vector<int>());  // 1 empty list per vertex (idx 0 unused)
    for (int n = 1; n <= tcount; n++) {                 // for every triangle
        for (int k = 0; k < 3; k++) {                   // each vertex of the triangle
            vertexTriangles[tlist[n][k]].push_back(n);  // triangle n touches this vertex
        }
    }
    
    /* Computing fnlist */
    for (int n = 1; n <= tcount; n++) {
        fnlist[n][0] = fnext(makeOrTri(n, 0));
        fnlist[n][1] = fnext(enext(makeOrTri(n, 0)));
        fnlist[n][2] = fnext(enext(enext(makeOrTri(n, 0))));
    }
    
    /* Compute the number of components */
    int numberOfComponents = computeNumberOfComponents();
    cout << "Number of Components = " << numberOfComponents << endl;
    
    computeVertexNormal();

}



void myObjType::computeStat()
{
	int i;
    double minAngle = 180;
    double maxAngle = 0;
    
    /* Compute Angle Statistics */
    for (int n = 1; n <= tcount; n++) {
        int v1, v2, v3; // id of the vertices
        v1 = tlist[n][0];
        v2 = tlist[n][1];
        v3 = tlist[n][2];
        
        double u[3], v[3], w[3];
        
        for (int k = 0; k < 3; k++) {
            u[k] = vlist[v2][k] - vlist[v1][k]; // v2 - v1
            v[k] = vlist[v3][k] - vlist[v2][k]; // v3 - v2
            w[k] = vlist[v1][k] - vlist[v3][k]; // v1 - v3
        }
        
        // magnitude
        double mag_u = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        double mag_v = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        double mag_w = sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
        
        // Dot product
        double a, b, c;
        a = u[0] * (-w[0]) + u[1] * (-w[1]) + u[2] * (-w[2]); // u . (-w)
        b = -u[0] * v[0] - u[1] * v[1] - u[2] * v[2]; // (-u) . v
        c = -v[0] * w[0] - v[1] * w[1] - v[2] * w[2]; // (-v) . w
        
        double angle_a = acos(a / (mag_u * mag_w)) * 180.0 / M_PI;
        double angle_b = acos(b / (mag_u * mag_v)) * 180.0 / M_PI;
        double angle_c = acos(c / (mag_v * mag_w)) * 180.0 / M_PI;
        
        double min_angle = min(min(angle_a, angle_b), angle_c);
        double max_angle = max(max(angle_a, angle_b), angle_c);
        
        minAngle = min(minAngle, min_angle);
        maxAngle = max(maxAngle, max_angle);
        
        if (isnan(min_angle) || isnan(max_angle)) {
            continue; // degenerate triangle (zero-length edge)
        }
        
        statMinAngle[(int) (min_angle / 10)]++;
        statMaxAngle[min((int) (max_angle / 10), 17)]++;    // 180 degrees goes to the last bucket
    }

    
    cout << "Min. angle = " << minAngle << endl;
    cout << "Max. angle = " << maxAngle << endl;

	cout << "Statistics for Maximum Angles" << endl;
	for (i = 0; i < 18; i++)
		cout << statMaxAngle[i] << " ";
	cout << endl;
	cout << "Statistics for Minimum Angles" << endl;
	for (i = 0; i < 18; i++)
		cout << statMinAngle[i] << " ";
	cout << endl;
}



/* Oriented-triangle operators: enext(), sym(), org(), dest() */
inline OrTri enext(OrTri ot) {
    int id = idx(ot);
    int version = ver(ot);
    if (version ==  0 || version == 1) {
        version = version + 1;
    } else if (version == 2) {
        version = 0;
    } else if (version == 3) {
        version = 5;
    }  else if (version == 4 || version == 5) {
        version = version - 1;
    } else {
        return 0;
    }
    return makeOrTri(id, version);
}

inline OrTri sym(OrTri ot) {
    int id = idx(ot);
    int version = ver(ot);
    if (version < 3) {
        version = version + 3;
    } else if (version < 6) {
        version = version - 3;
    } else {
        return 0;
    }
    return makeOrTri(id, version);
}

int myObjType::org(OrTri ot) {
    int id = idx(ot);
    int version = ver(ot);
    
    if (version == 0 || version == 5) {
        return tlist[id][0];
    } else if (version == 1 || version == 3) {
        return tlist[id][1];
    } else if (version == 2 || version == 4) {
        return tlist[id][2];
    }
    
    return 0;
}

int myObjType::dest(OrTri ot) {
    int id = idx(ot);
    int version = ver(ot);
    
    if (version == 2 || version == 3) {
        return tlist[id][0];
    } else if (version == 0 || version == 4) {
        return tlist[id][1];
    } else if (version == 1 || version == 5) {
        return tlist[id][2];
    }
    
    return 0;
}

/* fnext(): the oriented triangle sharing this edge, found via the vertex-to-triangle index */
OrTri myObjType::fnext(OrTri ot) {
    int id = idx(ot);
    
    int v1 = org(ot);
    int v2 = dest(ot);
    
    for (size_t t = 0; t < vertexTriangles[v1].size(); ++t) {
        int k = vertexTriangles[v1][t];
        if (k == id) {
            continue;
        }
        
        for (int v = 0; v < 6; v++) {
            OrTri current = makeOrTri(k, v);
            if (v1 == org(current) && v2 == dest(current)) {
                return current;
            }
        }
    }
    return 0;
}

/* Count connected components with an iterative DFS over fnlist */
int myObjType::computeNumberOfComponents() {
    vector<bool> visited(tcount + 1, false);
    int numberOfComponents = 0;
    
    for (int i = 1; i <= tcount; ++i) {
        if (!visited[i]) {
            // DFS
            std::stack<int> stack;
            stack.push(i);
            visited[i] = true;
            
            while (!stack.empty()) {
                int current = stack.top();
                stack.pop();
                
                // Traverse adjacent triangles through fnlist
                for (int j = 0; j < 3; ++j) {
                    OrTri adjacent = fnlist[current][j];
                    int adjacentIndex = idx(adjacent);
                    if (adjacentIndex != 0 && !visited[adjacentIndex]) {
                        stack.push(adjacentIndex);
                        visited[adjacentIndex] = true;
                    }
                }
            }
            
            // Increment number of components
            numberOfComponents++;
        }
    }
    
    return numberOfComponents;
}

/* Vertex normals for smooth shading: average of adjacent face normals */
void myObjType::computeVertexNormal() {
    // Set vnlist values to 0.0
    for (int i = 1; i <= vcount; i++) {
        vnlist[i][0] = 0.0;
        vnlist[i][1] = 0.0;
        vnlist[i][2] = 0.0;
    }
    
    // Add triangle normals to each vertex normals
    for (int i = 1; i <= tcount; i++) {
        for (int k = 0; k < 3; k++) {
            for (int j = 0; j < 3; j++) {
                vnlist[tlist[i][k]][j] += nlist[i][j];
            }
        }
    }
    
    // Normalize
    for (int i = 1; i <= vcount; i++) {
        double magnitude = sqrt(vnlist[i][0] * vnlist[i][0] + vnlist[i][1] * vnlist[i][1] + vnlist[i][2] * vnlist[i][2]);
        vnlist[i][0] /= magnitude;
        vnlist[i][1] /= magnitude;
        vnlist[i][2] /= magnitude;
    }
}

/* Visualize boundary edges */
void myObjType::visualizeBoundaryEdges() {
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    
    glBegin(GL_LINES);
    for (int i = 1; i <= tcount; i++) {
        for (int j = 0; j < 3; j++) {
            if (fnlist[i][j] == 0) {
                glVertex3dv(vlist[org(makeOrTri(i, j))]);
                glVertex3dv(vlist[dest(makeOrTri(i, j))]);
            }
        }
    }
    glEnd();
}

void myObjType::toggleBoundaryEdges() {
    isBoundaryEdgesVisible = !isBoundaryEdgesVisible;
}

/* Draw each connected component in its own colour */
void myObjType::highlightComponents() {
    vector<bool> visited(tcount + 1, false);
    
    srand(randomSeed);
    float componentColor[3] = {(float) rand() / RAND_MAX, (float) rand() / RAND_MAX, (float) rand() / RAND_MAX};
    
    vector<int> indices; // vertex ids of the current component's triangles
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_DOUBLE, 0, vlist);
    
    for (int i = 1; i <= tcount; ++i) {
        if (!visited[i]) {
            // DFS
            std::stack<int> stack;
            stack.push(i);
            visited[i] = true;
            
            glColor3fv(componentColor);
            indices.clear();
            while (!stack.empty()) {
                int current = stack.top();
                stack.pop();
                
                // Collect current triangle
                for (int j = 0; j < 3; ++j) {
                    indices.push_back(tlist[current][j]);
                }
                
                // Traverse adjacent triangles through fnlist
                for (int j = 0; j < 3; ++j) {
                    OrTri adjacent = fnlist[current][j];
                    int adjacentIndex = idx(adjacent);
                    if (adjacentIndex != 0 && !visited[adjacentIndex]) {
                        stack.push(adjacentIndex);
                        visited[adjacentIndex] = true;
                    }
                }
            }
            glDrawElements(GL_TRIANGLES, (GLsizei) indices.size(), GL_UNSIGNED_INT, &indices[0]);
            
            // Update component color for the next component
            componentColor[0] = (float) rand() / RAND_MAX;
            componentColor[1] = (float) rand() / RAND_MAX;
            componentColor[2] = (float) rand() / RAND_MAX;
        }
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

void myObjType::toggleHighlight() {
    isHighlight = !isHighlight;
}

void myObjType::changeHighlightColour() {
    randomSeed = rand();
}
