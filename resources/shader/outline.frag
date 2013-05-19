uniform sampler2D s_Data;
varying vec2 v_texCoord2D;
uniform vec2 v_invScreenSize;
uniform float f_outlineThickness;

const vec3 v_selectionColor = vec3(1.0,0.7,0.0);

float unpack_id(vec4 c) {
  return (c.r + c.g*256.0 + c.b*256.0*256.0 + c.a*256.0*256.0*256.0)*255.0;
}

void main(void) {
  //Get color of nearby points
  vec4 c00 = texture2D(s_Data, vec2(v_texCoord2D.x-v_invScreenSize.x*f_outlineThickness,v_texCoord2D.y));
  vec4 c01 = texture2D(s_Data, vec2(v_texCoord2D.x+v_invScreenSize.x*f_outlineThickness,v_texCoord2D.y));
  vec4 c10 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y-v_invScreenSize.y*f_outlineThickness));
  vec4 c11 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y+v_invScreenSize.y*f_outlineThickness));
  
  //Get values of nearby points
  float v00 = unpack_id(c00);
  float v01 = unpack_id(c01);
  float v10 = unpack_id(c10);
  float v11 = unpack_id(c11);

  //Contour shader
  float dx = v00 - v01; //Cross-section based contour
  float dy = v10 - v11;
  float d = sqrt(dx*dx+dy*dy);

  float contour = 0.0;
  if (d > 0.0) {
    contour = 1.0;
  }
  
  //Get color for the contour
//  float objectSelected = min(1.0,c00.g + c01.g + c10.g + c11.g);
  
//  gl_FragColor = vec4(dx*100.0,dy*100.0,d*100.0,1.0);
  gl_FragColor = vec4(0,0,0,contour);
//  gl_FragColor = vec4(c00.a*255.0,c00.a*255.0,c00.a*255.0,1.0);
//  gl_FragColor = vec4(texture2D(s_Data,v_texCoord2D).xyz,1.0);
}
