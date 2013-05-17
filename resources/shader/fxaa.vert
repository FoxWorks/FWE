varying vec4 vertTexcoord;

void main(void) {
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  vertTexcoord = gl_MultiTexCoord0;
}
