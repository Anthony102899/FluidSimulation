#version 330 core
in vec2 texCoord;
out vec4 FragColor;
uniform sampler2D boundary_texture;	//boundary texture map specifying boundaries
uniform sampler2D state_texture3;	    //input texture containing f0, rho, ux and uy
uniform vec2 mousePos;

void main()
{
	//	TO DO: More sophisticated display output

  	vec2 pos = texCoord.xy;		//	Position of each lattice node	
	//	Following are for dummy display
	if ( texture( boundary_texture,pos ).x > 0.5 ){
		float color = texture2D( state_texture3, pos ).y;
		FragColor = vec4( color*0.4, color*0.6, color, 0.0 );
	}
	else{
		FragColor = vec4( 0.0, 0.0, 0.0, 0.0 );
	}
	
}