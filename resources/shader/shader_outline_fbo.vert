varying vec2 v_texCoord2D;

void main(void) {
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
  v_texCoord2D = gl_MultiTexCoord0.xy;
}
