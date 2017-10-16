var path = require('path');
var webpack = require('webpack');

var CommonsChunkPlugin = webpack.optimize.CommonsChunkPlugin;
var autoprefixer = require('autoprefixer');
var HtmlWebpackPlugin = require('html-webpack-plugin');
var ExtractTextPlugin = require('extract-text-webpack-plugin');
var CopyWebpackPlugin = require('copy-webpack-plugin');
var CompressionPlugin = require("compression-webpack-plugin");
var CleanWebpackPlugin = require("clean-webpack-plugin");
var HtmlWebpackInlineSVGPlugin = require('html-webpack-inline-svg-plugin');
var ConcatPlugin = require('webpack-concat-plugin');
var HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');
var RawBundlerPlugin = require('webpack-raw-bundler');
var PreloadWebpackPlugin = require('preload-webpack-plugin');

var ENV = process.env.npm_lifecycle_event;
var isTestWatch = ENV === 'test-watch';
var isTest = ENV === 'test' || isTestWatch;
var isProd = ENV === 'build';

module.exports = function makeWebpackConfig() {
    var config = {};

    if (isProd) {
        config.devtool = 'source-map';
    } else if (isTest) {
        config.devtool = 'inline-source-map';
    } else {
        config.devtool = 'eval-source-map';
    }

    if (!isTest) {
        /*
                config.entry = isTest ? {} : {
                    'polyfills': './src_web/polyfills.ts',
                    'vendor': './src_web/vendor.ts',
                    'app': './src_web/main.ts'
                };
        */
        config.entry = isTest ? {} : {
            'app': ['./src_web/polyfills.ts', './src_web/vendor.ts', './src_web/main.ts']
        };
    }

    config.output = isTest ? {} : {
        path: root('data'),
        publicPath: isProd ? '/' : 'http://localhost:8080/',
        filename: isProd ? 'js/[name].js' : 'js/[name].js',
        chunkFilename: isProd ? '[id].chunk.js' : '[id].chunk.js'
    };

    config.resolve = {
        extensions: ['.ts', '.js', '.json', '.css', '.scss', '.html']
    };

    var atlOptions = '';
    if (isTest && !isTestWatch) {
        atlOptions = 'inlineSourceMap=true&sourceMap=false';
    }

    config.module = {
        rules: [{
            test: /\.ts$/,
            loaders: ['awesome-typescript-loader?' + atlOptions, 'angular2-template-loader', '@angularclass/hmr-loader'],
            exclude: [isTest ? /\.(e2e)\.ts$/ : /\.(spec|e2e)\.ts$/, /node_modules\/(?!(ng2-.+))/]
        }, {
            test: /\.(png|jpe?g|gif|svg|woff|woff2|ttf|eot|ico)(\?v=[0-9]\.[0-9]\.[0-9])?$/,
            loader: 'file-loader?name=fonts/[name].[ext]?'
        }, {
            test: /\.json$/,
            loader: 'json-loader'
        }, {
            test: /\.css$/,
            exclude: root('src_web', 'app'),
            loader: isTest ? 'null-loader' : ExtractTextPlugin.extract({
                fallback: 'style-loader',
                use: ['css-loader', 'postcss-loader']
            })
        }, {
            test: /\.css$/,
            include: root('src_web', 'app'),
            loader: 'raw-loader!postcss-loader'
        }, {
            test: /\.(scss|sass)$/,
            exclude: root('src_web', 'app'),
            loader: isTest ? 'null-loader' : ExtractTextPlugin.extract({
                fallback: 'style-loader',
                use: [{
                    loader: 'css-loader',
                    options: {
                        minimize: true
                    }
                }, 'postcss-loader', 'sass-loader']
            })
        }, {
            test: /\.(scss|sass)$/,
            exclude: root('src_web', 'css'),
            loader: 'raw-loader!postcss-loader!sass-loader'
        }, {
            test: /\.html$/,
            loader: 'raw-loader',
            exclude: root('src_web', 'public')
        }]
    };

    if (isTest && !isTestWatch) {
        config.module.rules.push({
            test: /\.ts$/,
            enforce: 'post',
            include: path.resolve('src_web'),
            loader: 'istanbul-instrumenter-loader',
            exclude: [/\.spec\.ts$/, /\.e2e\.ts$/, /node_modules/]
        });
    }

    if (!isTest || !isTestWatch) {
        // tslint support
        config.module.rules.push({
            test: /\.ts$/,
            enforce: 'pre',
            loader: 'tslint-loader'
        });
    }
    config.plugins = [
        new webpack.DefinePlugin({
            'process.env': {
                ENV: JSON.stringify(ENV)
            }
        }),

        new webpack.ContextReplacementPlugin(
            /angular(\\|\/)core(\\|\/)@angular/,
            root('./src')
        ),

        new webpack.LoaderOptionsPlugin({
            options: {
                tslint: {
                    emitErrors: false,
                    failOnHint: false
                },
                sassLoader: {
                    //includePaths: [path.resolve(__dirname, "node_modules/foundation-sites/scss")]
                    outputStyle: 'compressed',
                },
                postcss: [
                    autoprefixer({
                        browsers: ['last 2 version']
                    })
                ]
            }
        }),

        new webpack.ProvidePlugin({
            jQuery: 'jquery',
            $: 'jquery',
            jquery: 'jquery'
        })
    ];

    if (!isTest && !isTestWatch) {
        config.plugins.push(
            /*
            new CommonsChunkPlugin({
                name: ['vendor', 'polyfills']
                //async: true,
            }),
            */
            new HtmlWebpackPlugin({
                template: './src_web/public/index.html',
                chunksSortMode: 'dependency',
                inlineSource: '.(js|css)$'
            }),
            new ExtractTextPlugin({ filename: 'css/[name].css', disable: !isProd }),
            //new HtmlWebpackInlineSVGPlugin(),
            /*
            new PreloadWebpackPlugin({
                rel: 'preload',
                include: 'all'
            }),
            */
            new HtmlWebpackInlineSourcePlugin()//,
        );
    }

    if (isProd) {
        config.plugins.push(
            new webpack.NoEmitOnErrorsPlugin(),

            // // Reference: http://webpack.github.io/docs/list-of-plugins.html#dedupeplugin
            // // Dedupe modules in the output
            //new webpack.optimize.DedupePlugin(),

            new webpack.optimize.AggressiveMergingPlugin(),
            new webpack.optimize.UglifyJsPlugin({
                parallel: true,
                uglifyOptions: {
                    ie8: false,
                    ecma: 6,
                    warnings: true,
                    mangle: true, // debug false
                    output: {
                        comments: false,
                        beautify: false  // debug true
                    }
                },
                warnings: true
            }),

            /*
                        new ConcatPlugin({
                            uglify: true,
                            useHash: false,
                            sourceMap: false,
                            name: 'app',
                            fileName: '[name].bundle.js',
                            filesToConcat: ['./data/js/polyfills.js', './data/js/vendor.js', './data/js/app.js']
                        }),
            */
            /*
                        new RawBundlerPlugin({
                            bundles: [ "vendor.js" ],
                            "vendor.js": [ 'js/*.js' ]
                        }),
            */
            new CopyWebpackPlugin([{
                from: root('src_web/public'),
                //ignore: ['logo.svg']
            }])
            /*
                        new CompressionPlugin({
                            asset: "[path].gz[query]",
                            algorithm: "gzip",
                            test: /\.(js|css|html|svg)$/,
                            threshold: 0,
                            minRatio: 0,
                            deleteOriginalAssets: true
                        })
            */
            //new CleanWebpackPlugin(['js', 'css'], {
            //  root: './data',
            //  verbose: false,
            //  dry: false,
            //  exclude: ['**/*.js.gz']
            //})
            /*
            new CompressionPlugin({
                asset: "[path].gz[query]",
                algorithm: "gzip",
                test: /\.(html|svg)$/,
                threshold: 0,
                minRatio: 0.8,
                deleteOriginalAssets: true
            })//,
            */
        );
    }

    config.devServer = {
        contentBase: './src_web/public',
        historyApiFallback: true,
        quiet: true,
        stats: 'minimal' // none (or false), errors-only, minimal, normal (or true) and verbose
    };

    return config;
}();

function root(args) {
    args = Array.prototype.slice.call(arguments, 0);
    return path.join.apply(path, [__dirname].concat(args));
}
