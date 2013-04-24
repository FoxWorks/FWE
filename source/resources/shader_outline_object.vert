varying float f_Depth;
varying vec4 v_Color;
varying vec4 v_Position;
//uniform mat4 m_invView;
uniform mat4 m_View;

void main(void) {
  gl_Position = gl_ProjectionMatrix * m_View * gl_ModelViewMatrix * gl_Vertex;
  v_Position = gl_ModelViewMatrix * gl_Vertex;
  f_Depth = -gl_Position.z;
  v_Color = gl_Color;
}
