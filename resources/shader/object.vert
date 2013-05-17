varying vec3 v_worldNormal;
varying vec4 v_Data;
varying vec4 v_Position;
//uniform mat4 m_invView;
uniform mat4 m_View;

void main(void) {
  mat4 modelViewMatrix = m_View * gl_ModelViewMatrix;
  
  gl_Position = gl_ProjectionMatrix * modelViewMatrix * gl_Vertex;
  v_Position = gl_ModelViewMatrix * gl_Vertex;
  
  mat3 normalMatrix = mat3(
    modelViewMatrix[0][0], modelViewMatrix[0][1], modelViewMatrix[0][2],
    modelViewMatrix[1][0], modelViewMatrix[1][1], modelViewMatrix[1][2],
    modelViewMatrix[2][0], modelViewMatrix[2][1], modelViewMatrix[2][2]);
  
  v_worldNormal = normalize(normalMatrix * gl_Normal);
  v_Data = gl_Color;
}
