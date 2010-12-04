class mesh:
	def __init__(self):
		self.position = []
		self.normal = []
		self.uvw = []
		self.faces = []
		self.texture = []
		self.smoothing = []
class program:
	def __init__(self):
		self.meshes = {}
	def open(self,path):
		importFile = open(path)
		findName = False
		current = None
		for line in importFile:
			line = line.replace("\t"," ").replace("\n","")
			if "GEOMOBJECT" in line:
				current = None
				findName = True
			elif "NODE_NAME" in line and findName:
				findName = False
				name = line.split("NAME ")[-1].replace('"',"")
				current = self.meshes[name] = mesh()
			elif current:
				if "MESH_NUMVERTEX " in line:
					data = line.split("MESH_NUMVERTEX")[-1]
					current.normal = range(int(data))
				elif "MESH_VERTEX " in line:
					data = line.split("MESH_VERTEX")[-1].strip().split(" ")[1:]
					current.position.append(data)
				elif "MESH_TVERT " in line:
					data = line.split("MESH_TVERT")[-1].strip().split(" ")[1:]
					current.uvw.append(data)
				elif "MESH_TFACE " in line:
					data = line.split("MESH_TFACE")[-1].strip().split(" ")[1:]
					current.texture.append(data)
				elif "MESH_FACE " in line:
					data = line.split("MESH_FACE")[-1]
					A = data.split(" A:")[-1].split(" B: ")[0].strip()
					B = data.split(" B:")[-1].split(" C: ")[0].strip()
					C = data.split(" C:")[-1].split(" AB: ")[0].strip()
					smoothing = data.split("MESH_SMOOTHING ")[-1].split("*MESH_MTLID")[0].strip()
					current.smoothing.append(smoothing)
					current.faces.append([A,B,C])
				elif "MESH_VERTEXNORMAL " in line:
					base = line.split("MESH_VERTEXNORMAL ")[-1].strip().split(" ")
					current.normal[int(base[0])] = base[1:]
	def save(self,path):
		output = open(path,"w")
		meshes = self.meshes.items()
		meshes.reverse()
		vertexOffset = faceOffset = 0
		for name,mesh in meshes:
			output.write("g\n")
			output.write("# object "+name+"\n")
			output.write("#\n")
			for position in mesh.position:
				altered = "%f" % (float(position[1])*-1.0)
				output.write("v "+position[0]+" "+position[2]+" "+altered+"\n")
			output.write("# "+str(len(mesh.position))+" vertices\n\n")
			for uvw in mesh.uvw:
				output.write("vt "+uvw[0]+" "+uvw[1]+" "+uvw[2]+"\n")
			output.write("# "+str(len(mesh.uvw))+" texture vertices\n\n")
			for normal in mesh.normal:
				altered = "%f" % (float(normal[1])*-1.0)
				output.write("vn "+normal[0]+" "+normal[2]+" "+altered+"\n")
			output.write("# "+str(len(mesh.normal))+" vertex normals\n\n")
			output.write("g "+name+"\n")
			faceIndex = 0
			smoothing = mesh.smoothing[0]
			factor = int(smoothing)
			output.write("s "+smoothing+"\n")
			for face in mesh.faces:
				if smoothing != mesh.smoothing[faceIndex]:
					factor *= 2
					smoothing = mesh.smoothing[faceIndex]
					output.write("s "+str(factor)+"\n")
				set1A = str(int(face[0])+1+vertexOffset)
				set1B = str(int(mesh.texture[faceIndex][0])+1+faceOffset)
				set2A = str(int(face[1])+1+vertexOffset)
				set2B = str(int(mesh.texture[faceIndex][1])+1+faceOffset)
				set3A = str(int(face[2])+1+vertexOffset)
				set3B = str(int(mesh.texture[faceIndex][2])+1+faceOffset)
				set1 = set1A+"/"+set1B+"/"+set1A+" "
				set2 = set2A+"/"+set2B+"/"+set2A+" "
				set3 = set3A+"/"+set3B+"/"+set3A+" "
				output.write("f "+set1+set2+set3+"\n")
				faceIndex += 1
			output.write("# "+str(len(mesh.faces))+" faces\n\n")
			faceOffset += len(mesh.faces)
			vertexOffset += len(mesh.position)
		output.write("g\n\n")
main = program()
main.open("stuff.ASE")
main.save("dream.obj")