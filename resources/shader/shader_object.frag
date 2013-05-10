varying vec3 v_worldNormal;
varying vec4 v_Data;
varying vec4 v_Position;
uniform vec4 v_ClipPlane;

const vec3 v_viewDirection      = vec3(0,0,1);
const vec3 v_lightDirection     = vec3(0.1,0,1);

const float f_Ambient = 0.1;
const float f_Shininess = 4.0;
//const vec3 v_diffuseColor  = 0.70*vec3(1.00,1.00,1.00);
const vec3 v_specularColor = 0.20*vec3(1.00,1.00,1.00);

const vec3 v_sectionSelection = vec3(0,0,1);
const vec3 v_objectSelection = vec3(1,0,0);

void main(void) {
  float sectionSelected = v_Data.z*v_Data.x;
  float objectSelected = v_Data.x;
  float selectedColor = v_Data.w;
  
  vec3 v_diffuseColor = 0.70*vec3(
    mod(      selectedColor     ,2.0),
    mod(floor(selectedColor/2.0),2.0),
    mod(floor(selectedColor/4.0),2.0));

  //Compute lighting
  vec3 lightDirection = -normalize(v_lightDirection);
  float diffuse = abs(dot(v_worldNormal,lightDirection)) + f_Ambient;
  float specular = pow(max(0.0,dot(reflect(lightDirection,v_worldNormal),v_viewDirection)),f_Shininess);

  //Compute final color
  vec3 finalColor = vec3(0.0,0.0,0.0);
  finalColor += v_diffuseColor*diffuse;
  finalColor += v_specularColor*specular;
  
  //Object selection
  //finalColor -= v_objectSelection*diffuse*0.2*objectSelected;
  
  //Section selection
  finalColor *= (1.0-sectionSelected);
  finalColor += sectionSelected*v_sectionSelection*diffuse;

  //Process clip plane
  if (dot(v_ClipPlane,v_Position) < 0.0) {
    discard;
  } else {
    gl_FragColor = vec4(finalColor,1);
  }
}
