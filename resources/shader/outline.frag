uniform sampler2D s_Data;
varying vec2 v_texCoord2D;
uniform vec2 v_invScreenSize;
uniform float f_outlineThickness;

const vec3 v_selectionColor = vec3(1.0,0.7,0.0);

float unpack_id(vec4 c) {
  return (c.r + c.g*256.0 + c.b*256.0*256.0 + c.a*256.0*256.0*256.0)*255.0;
}

float outline() {
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
  float dx = v00 - v01;
  float dy = v10 - v11;
  float d = sqrt(dx*dx+dy*dy);

  //Edge detection
  if (d > 0.0) {
    return 1.0;
  }
  return 0.0;
}

float outline_aa() {
  //Get color of nearby points
  vec4 c00 = texture2D(s_Data, vec2(v_texCoord2D.x-v_invScreenSize.x*0.5*f_outlineThickness,v_texCoord2D.y));
  vec4 c01 = texture2D(s_Data, vec2(v_texCoord2D.x+v_invScreenSize.x*0.5*f_outlineThickness,v_texCoord2D.y));
  vec4 c10 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y-v_invScreenSize.y*0.5*f_outlineThickness));
  vec4 c11 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y+v_invScreenSize.y*0.5*f_outlineThickness));

  vec4 d00 = texture2D(s_Data, vec2(v_texCoord2D.x-v_invScreenSize.x*1.0*f_outlineThickness,v_texCoord2D.y));
  vec4 d01 = texture2D(s_Data, vec2(v_texCoord2D.x+v_invScreenSize.x*1.0*f_outlineThickness,v_texCoord2D.y));
  vec4 d10 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y-v_invScreenSize.y*1.0*f_outlineThickness));
  vec4 d11 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y+v_invScreenSize.y*1.0*f_outlineThickness));
  
  vec4 e00 = texture2D(s_Data, vec2(v_texCoord2D.x-v_invScreenSize.x*1.5*f_outlineThickness,v_texCoord2D.y));
  vec4 e01 = texture2D(s_Data, vec2(v_texCoord2D.x+v_invScreenSize.x*1.5*f_outlineThickness,v_texCoord2D.y));
  vec4 e10 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y-v_invScreenSize.y*1.5*f_outlineThickness));
  vec4 e11 = texture2D(s_Data, vec2(v_texCoord2D.x,v_texCoord2D.y+v_invScreenSize.y*1.5*f_outlineThickness));

  //Get values of nearby points
  float u00 = unpack_id(c00);
  float u01 = unpack_id(c01);
  float u10 = unpack_id(c10);
  float u11 = unpack_id(c11);
  
  float v00 = unpack_id(d00);
  float v01 = unpack_id(d01);
  float v10 = unpack_id(d10);
  float v11 = unpack_id(d11);
  
  float w00 = unpack_id(e00);
  float w01 = unpack_id(e01);
  float w10 = unpack_id(e10);
  float w11 = unpack_id(e11);

  //Contour shader
  float d1x = u00 - u01;
  float d1y = u10 - u11;
  float d1 = sqrt(d1x*d1x+d1y*d1y);
  
  float d2x = v00 - v01;
  float d2y = v10 - v11;
  float d2 = sqrt(d2x*d2x+d2y*d2y);
  
  float d3x = w00 - w01;
  float d3y = w10 - w11;
  float d3 = sqrt(d3x*d3x+d3y*d3y);

  //Edge detection
  float contour = 0.0;
  if (d1 > 0.0) contour += 0.25;
  if (d2 > 0.0) contour += 0.50;
  if (d3 > 0.0) contour += 0.25;
  return contour;
}

void main(void) {
  float contour = outline_aa();
  gl_FragColor = vec4(0,0,0,contour);
  
//  gl_FragColor = vec4(dx*100.0,dy*100.0,d*100.0,1.0);
//  gl_FragColor = vec4(c00.a*255.0,c00.a*255.0,c00.a*255.0,1.0);
//  gl_FragColor = vec4(texture2D(s_Data,v_texCoord2D).xyz,1.0);
}
