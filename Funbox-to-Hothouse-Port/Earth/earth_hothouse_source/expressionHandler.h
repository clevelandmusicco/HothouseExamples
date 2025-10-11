// Expression Handler for Earth Hothouse
// Manages expression pedal mapping to knob parameters

#ifndef EXPRESSION_HANDLER_H
#define EXPRESSION_HANDLER_H

class ExpressionHandler {
private:
    bool expression_set_mode;
    int num_params;
    float* heel_values;
    float* toe_values;
    bool* param_active;
    float led1_brightness;
    float led2_brightness;
    
public:
    ExpressionHandler() : expression_set_mode(false), num_params(0), 
                          heel_values(nullptr), toe_values(nullptr), 
                          param_active(nullptr), led1_brightness(0.1f), 
                          led2_brightness(0.1f) {}
    
    void Init(int params) {
        num_params = params;
        heel_values = new float[num_params];
        toe_values = new float[num_params];
        param_active = new bool[num_params];
        
        for(int i = 0; i < num_params; i++) {
            heel_values[i] = 0.0f;
            toe_values[i] = 1.0f;
            param_active[i] = false;
        }
    }
    
    ~ExpressionHandler() {
        if(heel_values) delete[] heel_values;
        if(toe_values) delete[] toe_values;
        if(param_active) delete[] param_active;
    }
    
    void Process(float expression_value, float* knob_values, float* output_values) {
        for(int i = 0; i < num_params; i++) {
            if(param_active[i]) {
                // Linear interpolation between heel and toe values
                output_values[i] = heel_values[i] + (toe_values[i] - heel_values[i]) * expression_value;
            } else {
                // Pass through knob value if not controlled by expression
                output_values[i] = knob_values[i];
            }
        }
        
        if(expression_set_mode) {
            // Update LED brightness based on expression position
            led1_brightness = 0.1f + 0.4f * expression_value;
            led2_brightness = 0.1f + 0.4f * (1.0f - expression_value);
        }
    }
    
    void ToggleExpressionSetMode() {
        expression_set_mode = !expression_set_mode;
    }
    
    bool isExpressionSetMode() {
        return expression_set_mode;
    }
    
    float returnLed1Brightness() {
        return led1_brightness;
    }
    
    float returnLed2Brightness() {
        return led2_brightness;
    }
    
    void Reset() {
        expression_set_mode = false;
        for(int i = 0; i < num_params; i++) {
            heel_values[i] = 0.0f;
            toe_values[i] = 1.0f;
            param_active[i] = false;
        }
    }
    
    void SetParameterActive(int param, bool active) {
        if(param < num_params) {
            param_active[param] = active;
        }
    }
    
    void SetHeelValue(int param, float value) {
        if(param < num_params) {
            heel_values[param] = value;
        }
    }
    
    void SetToeValue(int param, float value) {
        if(param < num_params) {
            toe_values[param] = value;
        }
    }
};

#endif // EXPRESSION_HANDLER_H
